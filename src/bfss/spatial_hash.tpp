#pragma once
#include "bfss.tpp"
#include "../okvs.tpp"
#include <unordered_map>
#include "dbg.h"
// a spatial hash is something that takes 2D (non-negative) integer coordinates, and obliviously return bitstrings (usually bFSS half-shares).
// this class does not take care of smaller construction, you have to supply serialised sub-bFSS to it. 
template<uint64_t KeyBitLength, uint64_t ValueLength, uint64_t Lambda>
struct SpatialHash {
public:
    const static uint64_t KeyLength = 2 * KeyBitLength; // recall we have two keys: X and Y
    const static uint64_t OutputSize = ValueLength * (KeyLength + Lambda) + Lambda; // this is the calculated length of final output
    using SerialisedKey = std::bitset<KeyLength>; 
    using Value = std::bitset<ValueLength>;

    // instantiate okvs with our required parameters
    using SuitableOkvs = okvs::RandomBooleanPaXoS<KeyLength, ValueLength, Lambda>;

    using EncodedPaXoS = SuitableOkvs::EncodedPaXoS;
    using Nonce = SuitableOkvs::Nonce;

    SpatialHash() {
        static_assert(KeyBitLength <= 31); // to simplify below logic. (2 ** 31) ** 2 should be more than enough
    }

    // as encoder
    void insert(uint32_t x, uint32_t y, const Value& value) {
        kvs[serialize(x, y)] = value;
    }
    
    std::optional<std::bitset<OutputSize>> encode() {
        auto randomSource = std::make_unique<std::random_device>();
        SuitableOkvs okvs(std::move(randomSource));

        std::vector<std::pair<SerialisedKey, Value>> kvs_;
        std::copy(kvs.begin(), kvs.end(), std::back_inserter(kvs_));

        return okvs.serialize(okvs.encode(kvs_));
    }

    // as decoder, we wish to decode;
    Value decode(const std::bitset<OutputSize>& str, uint32_t x, uint32_t y, bool dbgg = false) {
        auto randomSource = std::make_unique<std::random_device>();
        SuitableOkvs okvs(std::move(randomSource));
        auto [paxos, nonce] = okvs.deserialize(str);
        if (dbgg) dbg(paxos), dbg(nonce), dbg(serialize(x, y));
        return okvs.decode(paxos, nonce, serialize(x, y), dbgg);
    }

    constexpr static size_t getOutputSize() {
        return OutputSize;
    }
protected:
    SerialisedKey serialize(uint32_t x, uint32_t y) {
        uint32_t maskLastKBit = (1 << KeyBitLength) - 1;
        uint32_t cleanX = maskLastKBit & x;
        uint32_t cleanY = maskLastKBit & y;
        uint32_t concat = (cleanX << KeyBitLength) | cleanY; // concat two ints together
        SerialisedKey key(concat);

        return key;
    }
    std::unordered_map<SerialisedKey, Value> kvs; // we use map instead of vector here to ensure key are pairwise different.
};