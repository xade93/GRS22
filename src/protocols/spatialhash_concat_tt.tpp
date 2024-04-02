#pragma once
#include "common.tpp"
// for abstract class of protocol
#include "protocol.tpp"

// for implementing protocol
#include "bfss/spatial_hash.tpp"
#include "bfss/trivial_bfss.tpp"
#include "oblivious_transfer.tpp"
#include <libOTe/TwoChooseOne/Iknp/IknpOtExtReceiver.h>
#include <libOTe/TwoChooseOne/Iknp/IknpOtExtSender.h>

// for transfer of fingerprints
#include <cryptoTools/Network/Session.h>
#include <cryptoTools/Network/IOService.h>
#include <cryptoTools/Network/Channel.h>

using std::string;

// GRS22's protocol, using spatialhash + concat + tt, over L-infinity norm.
// usage condition: Distance between Alice's balls >= 4 * (radius of balls)
// this has better performance than spatial hash + tt, see paper for detail.
// for role of server and client, see protocols/protocol.hpp
template<int bitLength, int Lambda, int L, int cellBitLength> 
class spatialhash_concat_tt: public GRS22_L_infinity_protocol<bitLength, Lambda, L, cellBitLength> {
public:
    using Point = std::pair<uint64_t, uint64_t>;
    // Set intersection server, holding structure and yielding intersection result. 
    // use Iknp as OT protocol on default.
    // @param centers       the vector of points storing center of Alice's balls.
    // @param clientIP      IP of client. Port is not needed and is setted above.
    // @param radius        radius of Alice's spheres.
    std::set<Point> SetIntersectionServer(const std::vector<Point>& centers, const string& clientIP, uint32_t radius)
    {
        static_assert(cellBitLength <= 32); // so that we can (conveniently) perform arithmetic in uint64_t. This should be more than enough, and since below have quadratic complexity, this number is already beyond enough.
        const uint64_t cellLength = 1ull << cellBitLength;
        // step 1. Alice generate L copies of bFSS describing her structure.
        
        // 1.1: first we partition all points in Alice into cells
        uint64_t AliceExpandedPointsCount = 0;
        std::map<Point, std::set<Point>> pointsByCell;
        for (uint64_t x = 0; x < (1ull << bitLength); ++x) {
            for (uint64_t y = 0; y < (1ull << bitLength); ++y) {
                if (this->membership(centers, x, y, radius)) { // this-> is necessary for disambiguation
                    pointsByCell[std::make_pair<uint64_t, uint64_t>(x / cellLength, y / cellLength)].emplace(x, y);
                    AliceExpandedPointsCount++;
                }
            }
        }
        std::cout << "Alice's structure contains in total " << AliceExpandedPointsCount << " points." << std::endl;

        // 1.2: now encode each cell into one OKVS, and insert into spatial hash; we repeat this process L times (and hence L time of OT later)
        // since OT only transfers std::bitset, we need to write serialise to bitset for our structure.
        const uint64_t singleTruthTableLength = cellLength;
        const uint64_t okvsKeyLength = 2 * (bitLength - cellBitLength); // key of okvs is (x / radius, y / radius)
        using SuitableSpatialHash = SpatialHash<bitLength - cellBitLength, singleTruthTableLength * 2, Lambda>;
        constexpr uint64_t shareLength = SuitableSpatialHash::getOutputSize();
        std::array<std::pair<std::bitset<shareLength>, std::bitset<shareLength>>, L> shares;

        for (uint64_t trial = 0; trial < L; ++trial) {
            SuitableSpatialHash h0, h1;
            for (auto& [key, pointSet]: pointsByCell) {
                const uint64_t l1 = bitLength - cellBitLength, l2 = cellBitLength;
                TruthTable<cellBitLength, 1> tt[2]; // d copies of truth table.
                std::set<uint64_t> activeLocations[2]; // note this differs from spatialhash + tt as we use need to deduplicate by key here

                for (auto& [u, v]: pointSet) {
                    activeLocations[0].emplace(u % cellLength); 
                    activeLocations[1].emplace(v % cellLength);
                }  

                // we prepare the vector used for encoding truth table.
                // below is just a simple changing from set to vector.
                std::vector<std::pair<uint64_t, std::bitset<1>>> cellDescription[2];
                for (uint64_t dim = 0; dim < 2; ++dim) 
                    for (auto elem: activeLocations[dim]) cellDescription[dim].emplace_back(elem, std::bitset<1>(0));

                // shareX0 shareX1 are the two copies of secret share of *only X coordinate*; similarly Y
                auto [shareX0, shareX1] = tt[0].encode(cellDescription[0]);
                auto [shareY0, shareY1] = tt[1].encode(cellDescription[1]);
                // we need to concat shareX0 with shareY0 , similar etc.
                h0.insert(key.first, key.second, concatBitSet(shareX0, shareY0));
                h1.insert(key.first, key.second, concatBitSet(shareX1, shareY1));
            }
            auto v0 = h0.encode().value(); // v0 and v1 are 2 shares of a representation of entire set of Alice.
            auto v1 = h1.encode().value();

            shares[trial] = std::make_pair(v0, v1);
        }


        // step 2. we perform OT for Bob to pick.
        TwoChooseOne_Sender<IknpOtExtSender, IknpOtExtReceiver, shareLength, L>(clientIP, shares);

        // step 3. Bob should done evaluating by now. Receive fingerprints from him
        const int blockCount = (2 * L + 127) / 128;
        std::vector<std::array<block, blockCount>> fingerprints; // TODO does this work?
        IOService ios;
        auto chl = Session(ios, clientIP + port, SessionMode::Client).addChannel();
        chl.recv(fingerprints);
        std::cout << "Estimated communication for Bob's fingerprints (bytes): " << channelBuffSize(fingerprints) << std::endl;

        // step 4. We also generate fingerprint for every element of ours with randomly selected s.
        std::random_device dev; std::mt19937_64 rng(dev());
        std::bitset<L> s = GetBitSequenceFromPRNG<L>(rng);

        std::unordered_map<std::bitset<2 * L>, std::pair<uint64_t, uint64_t>> restoration;
        for (uint64_t x = 0; x < (1ull << bitLength); ++x) {
            for (uint64_t y = 0; y < (1ull << bitLength); ++y) if (this->membership(centers, x, y, radius))  { // "this->" is necessary for disambiguation
                std::bitset<2 * L> fingerprint;
                for (uint64_t i = 0; i < L; ++i) {
                    SuitableSpatialHash hash;
                    auto inner = hash.decode((s[i] ? shares[i].second : shares[i].first), x / cellLength, y / cellLength);
                    auto [encodedX, encodedY] = splitBitSet<cellLength, cellLength>(inner);
                    TruthTable<cellBitLength, 1> truthTableX(encodedX), truthTableY(encodedY);

                    bool xbit = truthTableX.evaluate(x % cellLength)[0];
                    bool ybit = truthTableY.evaluate(y % cellLength)[0];

                    fingerprint[2 * i] = xbit, fingerprint[2 * i + 1] = ybit; // we need to pack each result (2 bit) into fingerprint (2L bit).
                }
                restoration[fingerprint] = std::make_pair(x, y); // TODO note here collision
            }
        }

        // step 5. look up the intersection between fingerprint of Alice's and Bob's, which yields result.
        std::set<Point> intersections;
        for (const auto& arr: fingerprints) {
            std::bitset<2 * L> fp = conversion_tools::blocksToBs<2 * L>(arr);

            if (restoration.find(fp) != restoration.end()) intersections.emplace(restoration[fp]);
        }
        return intersections;
    }

    // Set intersection client, holding unstructued points and yield nothing. 
    // use Iknp as OT protocol on default.
    // @param points        the vector of Bob's points.
    // @param serverIP      IP of server. Port is not needed and is setted above.
    void SetIntersectionClient(const std::vector<Point>& points, const string& serverIP) {
        const uint64_t cellLength = (1ull << cellBitLength);
        // First, we generate random sequence s
        std::random_device dev; std::mt19937_64 rng(dev());
        std::bitset<L> s = GetBitSequenceFromPRNG<L>(rng);

        // Next, we perform OT to select the L half-shares from Alice.
        using SuitableSpatialHash = SpatialHash<bitLength - cellBitLength, 2 * cellLength, Lambda>;
        constexpr uint64_t shareLength = SuitableSpatialHash::getOutputSize();
        auto ret = TwoChooseOne_Receiver<IknpOtExtSender, IknpOtExtReceiver, shareLength, L>(serverIP, s);
        
        // We evaluate each of our point against the L half-shares, yielding 2L-bit long "fingerprint" for each item of ours.
        // since cryptoTools only supports network transfer of blocks, we need to split our bitset into blocks again. 
        // In practice since error probability is (1 / 2) ^ L, L would hardly ever be larger than 128.
        const int blockCount = (2 * L + 127) / 128;
        std::vector<std::array<block, blockCount>> fingerprints(points.size());
        for (uint64_t pointIdx = 0; pointIdx < points.size(); ++pointIdx) {
            auto [x, y] = points[pointIdx];
            std::bitset<2 * L> fp;
            for (uint64_t idx = 0; idx < L; ++idx) {
                SuitableSpatialHash hash;
                auto inner = hash.decode(ret[idx], x / cellLength, y / cellLength);
                auto [encodedX, encodedY] = splitBitSet<cellLength, cellLength>(inner);
                TruthTable<cellBitLength, 1> truthTableX(encodedX), truthTableY(encodedY);

                bool xbit = truthTableX.evaluate(x % cellLength)[0];
                bool ybit = truthTableY.evaluate(y % cellLength)[0];

                fp[2 * idx] = xbit, fp[2 * idx + 1] = ybit; // we need to pack each result (2 bit) into fingerprint (2L bit).
            }
            fingerprints[pointIdx] = conversion_tools::bsToBlocks<2 * L>(fp);
        }

        // We transfer such fingerprints back to Alice. Bob's part is now complete.
        IOService ios;
        auto chl = Session(ios, serverIP + port, SessionMode::Server).addChannel();
        chl.send(std::move(fingerprints));
    }
protected:
    const std::string port = ":2468"; // start with :
    // aux function that takes last K bits of a 64 bit integer x.
    uint64_t lastKBits(uint64_t x, int K) {
        assert(K <= 64);
        if (K == 64) return x;
        else {
            return x % (1ull << K);
        }
    }
};