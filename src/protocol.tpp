#pragma once
#include "common.tpp"
#include "bfss/spatial_hash.tpp"
#include "bfss/trivial_bfss.tpp"
#include "oblivious_transfer.tpp"
#include <libOTe/TwoChooseOne/Iknp/IknpOtExtReceiver.h>
#include <libOTe/TwoChooseOne/Iknp/IknpOtExtSender.h>

using std::string;

// GRS22's protocol, using tt + spatialhash, over L-infinity norm.
// server is the one holding structure, and also the one receiving final intersection.
// client receive nothing on default.
// TODO explanation on arguments
namespace GRS22_L_infinity_protocol {
    using Point = std::pair<uint64_t, uint64_t>;
    const std::string port = ":2468"; // start with :
    // aux function
    uint64_t lastKBits(uint64_t x, int K) {
        assert(K <= 64);
        if (K == 64) return x;
        else {
            uint64_t mask = (1ull << K) - 1;
            return mask & x;
        }
    }

    // set intersection server, holding structure and yielding intersection result. using Iknp on default.
    template<int bitLength, int Lambda, int NumItem, int L, int radiusBitLength> 
    std::vector<Point> SetIntersectionServer(const std::vector<Point>& centers, const string& clientIP)
    {
        static_assert(radiusBitLength <= 32); // so that we can (conveniently) perform arithmetic in uint64_t. This should be more than enough, and since below have quadratic complexity, this number is already beyond enough.
        const uint64_t radius = 1ull << radiusBitLength;
        auto membership = [](const std::vector<Point>& centers, uint64_t x, uint64_t y) { // TODO optimize
            for (auto [currX, currY]: centers) {
                if (std::max(
                    std::abs((int64_t)currX - (int64_t)x), 
                    std::abs((int64_t)currY - (int64_t)y)) <= radius) return true;
            }
            return false;
        };

        // step 1. Alice generate L copies of bFSS describing her structure. TODO optimize
        
        // first we partition all points in Alice into cells
        std::map<Point, std::set<Point>> pointsByCell;
        for (uint64_t x = 0; x < (1ull << bitLength); ++x) {
            for (uint64_t y = 0; y < (1ull << bitLength); ++y) {
                if (membership(centers, x, y)) pointsByCell[std::make_pair<uint64_t, uint64_t>(x / radius, y / radius)].emplace(x, y);
            }
        }
        // now encode each cell into one OKVS, and insert into spatial hash; we repeat this process L times (and hence L time of OT later)
        // since OT only transfers std::bitset, we need to write serialise to bitset for our structure.
        const uint64_t truthTableLength = (1ull << (2 * radiusBitLength)); // by calculation we see inner TT size is exactly 4 ^ radiusBitLength. This is also validated by template system.
        const uint64_t okvsKeyLength = 2 * (bitLength - radiusBitLength); // this is also quite obvious
        const uint64_t shareLength = SpatialHash<bitLength, radius, Lambda>::getOutputSize();
        std::array<std::pair<std::bitset<shareLength>, std::bitset<shareLength>>, L> shares;

        for (uint64_t trial = 0; trial < L; ++trial) {
            SpatialHash<bitLength, radius, Lambda> h0, h1;
            for (auto& [key, pointSet]: pointsByCell) {
                const uint64_t l1 = bitLength - radiusBitLength, l2 = radiusBitLength;
                TruthTable<radiusBitLength * 2, 1> tt;
                std::vector<std::pair<std::bitset<radiusBitLength * 2>, std::bitset<1>>> cellDescription;

                for (auto& [x, y]: pointSet) {
                    auto encodedKeys = lastKBits(x, radiusBitLength) << radiusBitLength | lastKBits(y, radiusBitLength);
                    cellDescription.emplace_back(std::bitset<l1>(encodedKeys), std::bitset<1>(1));
                }

                auto [share0, share1] = tt.encode(cellDescription);
                h0.insert(key.first, key.second, share0);
                h1.insert(key.first, key.second, share1);
            }
            auto v0 = h0.encode().value(); // v0 and v1 are 2 shares of a representation of entire set of Alice.
            auto v1 = h1.encode().value();

            shares[trial] = std::make_pair(v0, v1);
        }

        // next, we perform OT for Bob to pick.
        TwoChooseOne_Sender<IknpOtExtSender, IknpOtExtReceiver, bitLength, L>(clientIP + port, shares);
    }

    template<int bitLength, int Lambda, int NumItem, int L, int radiusBitLength>
    void SetIntersectionClient(const std::vector<Point>& points, const string& serverIP) {
        const uint64_t radius = (1ull << radiusBitLength);
        // First, we generate random sequence s
        std::random_device dev; std::mt19937_64 rng(dev());
        std::bitset<L> s = GetBitSequenceFromPRNG<L>(rng);

        // Next, we perform OT to select the L half-shares from Alice.
        auto ret = TwoChooseOne_Receiver<IknpOtExtSender, IknpOtExtReceiver, bitLength, L>(serverIP + port);

        // We evaluate each of our point against the L half-shares, yielding L-bit long "fingerprint" for each item of ours.
        for (auto [u, v]: points) {
            for (uint64_t idx = 0; idx < L; ++idx) {
                // SpatialHash<bitLength, radius, Lambda> hash()
            }
        }
    }
}