#pragma once
#include "bfss.tpp"
#include "../okvs.tpp"

// a spatial hash is something that takes 2D (non-negative) integer coordinates, and obliviously return bitstrings (usually bFSS half-shares).
// this class does not take care of smaller construction, you have to supply serialised sub-bFSS to it. 
template<int KeyBitLength, int ValueLength, int Lambda>
struct SpatialHash {
public:
    const static int KeyLength = 2 * KeyBitLength; // recall we have two keys: X and Y
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
    
    std::optional<std::pair<EncodedPaXoS, Nonce>> encode() {
        auto randomSource = std::make_unique<std::random_device>();
        SuitableOkvs okvs(std::move(randomSource));

        std::vector<std::pair<SerialisedKey, Value>> kvs_;
        std::copy(kvs.begin(), kvs.end(), std::back_inserter(kvs_));

        return okvs.encode(kvs_);
    }

    // as decoder, we wish to decode;
    Value decode(const EncodedPaXoS& paxos, const Nonce& nonce, uint32_t x, uint32_t y) {
        auto randomSource = std::make_unique<std::random_device>();
        SuitableOkvs okvs(std::move(randomSource));

        return okvs.decode(paxos, nonce, serialize(x, y));
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