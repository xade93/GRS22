#pragma once
#include "common.tpp"
#include "bfss/spatial_hash.tpp"
#include "bfss/trivial_bfss.tpp"
#include "oblivious_transfer.tpp"

using std::string;

// GRS22's protocol, using tt + spatialhash, over L-infinity norm.
// server is the one holding structure, and also the one receiving final intersection.
// client receive nothing on default.
// TODO explanation on arguments
namespace GRS22_L_infinity_protocol {
    using Point = std::pair<uint64_t, uint64_t>;

    // aux function
    uint64_t lastKBits(uint64_t x, int K) {
        assert(K <= 64);
        if (K == 64) return x;
        else {
            uint64_t mask = (1ull << K) - 1;
            return mask & x;
        }
    }

    template<int bitLength, int Lambda, int NumItem, int L, int radiusBitLength> 
    std::vector<Point> SetIntersectionServer(const std::vector<Point>& centers, const string& clientIP)
    {
        static_assert(radiusBitLength <= 32); // so that we can perform arithmetic in uint64_t. This should be more than enough, and since below have quadratic complexity, this number is already beyond enough.
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
        std::map<Point, std::set<Point>> requiredPoints;
        for (uint64_t x = 0; x < (1ull << bitLength); ++x) {
            for (uint64_t y = 0; y < (1ull << bitLength); ++y) {
                if (membership(centers, x, y)) requiredPoints[std::make_pair<uint64_t, uint64_t>(x / radius, y / radius)].emplace(x, y);
            }
        }
        // now encode each cell into one OKVS, and insert into spatial hash; we repeat this process L times
        std::vector<std::pair<std::bitset
        for (uint64_t trial = 0; trial < L; ++trial) {
            SpatialHash<bitLength, radius, Lambda> spatialHash;
            for (auto& [key, pointSet]: requiredPoints) {
                const uint64_t l1 = bitLength - radiusBitLength, l2 = radiusBitLength;
                TruthTable<radiusBitLength * 2, 1> tt;
                std::vector<std::pair<std::bitset<radiusBitLength * 2>, std::bitset<1>>> cellDescription;

                for (auto& [x, y]: pointSet) {
                    auto encodedKeys = lastKBits(x, radiusBitLength) << radiusBitLength | lastKBits(y, radiusBitLength);
                    cellDescription.emplace_back(std::bitset<l1>(encodedKeys), std::bitset<1>(1));
                    auto encoded = tt.encode(cellDescription);
                    spatialHash.insert(x >> radiusBitLength, y >> radiusBitLength, encoded);
                }
            }
            auto encodedPaXoS = spatialHash.encode();

        }
    }

    template<int bitLength, int Lambda, int NumItem, int radius>
    void SetIntersectionClient(const std::vector<Point>& points, const string& serverIP) {
        
    }
}