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

// concats two bitsets; optimize not quite possible without not using std::bitset.
// order and endianness is consistent with splitBitSet
// performance analysis see https://stackoverflow.com/questions/3061721/concatenate-boostdynamic-bitset-or-stdbitset
template<size_t s1, size_t s2>
std::bitset<s1 + s2> concatBitSet(const std::bitset<s1>& u, const std::bitset<s2>& v) {
    std::bitset<s1 + s2> ret;
    for (size_t i = 0; i < s1; ++i) ret[i] = u[i];
    for (size_t i = s1; i < s1 + s2; ++i) ret[i] = v[i - s1];
    return ret;
}

// splits bitset into partition of size 2. the order and endianness of split is consistent with concatBitSet.
template<size_t s1, size_t s2>
std::pair<std::bitset<s1>, std::bitset<s2>> splitBitSet(const std::bitset<s1 + s2>& x) {
    std::bitset<s1> low; std::bitset<s2> high;
    for (size_t i = 0; i < s1; ++i) low[i] = x[i];
    for (size_t i = s1; i < s1 + s2; ++i) high[i - s1] = x[i];
    return std::make_pair(low, high);
}