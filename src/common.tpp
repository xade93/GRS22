#pragma once
#include <string>
#include <algorithm>
#include <map>
#include <cstdint>
#include <bitset>
#include <random>
#include <iostream>
#include <functional>
#include <memory>
#include <unordered_set>

// Generate pseudorandom bit sequence from PRNG source. 
// Note: it alters internal state of the PRNG.
// TODO more efficient way due to apparent bit alignment.
template<uint64_t L>
std::bitset<L> GetBitSequenceFromPRNG(std::mt19937_64& src) {
    std::bitset<L> bits;
    for (uint64_t idx = 0; idx < L; idx += 64) {
        auto val = src();
        std::bitset<64> currBatch(val);
        for (uint64_t currBit = idx; currBit < std::min(idx + 64, L); ++currBit) {
            bits[currBit] = currBatch[currBit - idx];
        }
    }
    return bits;
}
