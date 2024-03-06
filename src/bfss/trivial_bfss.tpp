#pragma once

#include "common.tpp"
#include "bfss/bfss.tpp"

// truth table (tt) bFSS described in paper.
// nonspecified keys will map to zero.
// this handles function whose required description size does not exceed uint64_t, which should be more than enough.
// due to this constraint, we can simplify key to uint64_t. value still have to be bitset as it can exceed 64 bits.
template<uint64_t KeyLength, uint64_t ValueLength>
class TruthTable: bFSS<uint64_t, std::bitset<ValueLength>, (1ull << KeyLength) * ValueLength> {
public:
    using BaseType = bFSS<uint64_t, std::bitset<ValueLength>, (1ull << KeyLength) * ValueLength>; 
    const static uint64_t ShareLength = (1ull << KeyLength) * ValueLength;
    using SecretShare = BaseType::SecretShare;
    using SecretPair = BaseType::SecretPair;
    using Key = uint64_t; 
    using Value = std::bitset<ValueLength>;
    // for encoder 
    TruthTable() {
        static_assert(KeyLength <= 32);
    };
    ~TruthTable() {};
    SecretPair encode(const std::vector<std::pair<Key, Value>>& data) {
        std::random_device dev;
        std::mt19937_64 rng(dev());
        // generate enough randomness for secret sharing, and fill onto secret shares
        auto pad = GetBitSequenceFromPRNG<ShareLength>(rng); 
        SecretPair ret;
        ret.first = ret.second = pad;

        // now XOR the values into pad; nonspecified keys will evaluate to 0 by default
        for (const auto& [Key, Value]: data) {
            for (uint64_t currBit = 0; currBit < ValueLength; ++currBit) {
                uint64_t offset = Key * ValueLength + currBit;
                ret.second[offset] = ret.second[offset] ^ Value[currBit];
            }
        }
        return ret;
    };
    // for evaluator
    TruthTable(const SecretShare& share_): BaseType(share_) {};
    Value evaluate(const Key& key) {
        Value ret;
        for (uint64_t currBit = 0; currBit < ValueLength; ++currBit) {
            uint64_t offset = key * ValueLength + currBit;
            ret[currBit] = this->share[offset]; 
        }
        return ret;
    };
};

