#pragma once

#include "common.tpp"
#include "bfss/bfss.tpp"

// xor-share bFSS as described in paper. This works iff the balls are "globally axis disjoint" i.e. the projection of spheres onto any of d axis are disjoint.
// specifically, we set the value length to ValueLength, so that this is (1 - 2 ^ {-V}, 1)-bFSS.
// the key type is uint64_t instead of bitset, since we are dealing with axis here, 64 bit should be more than enough.
template<uint64_t KeyLength, int ValueLength>
class XorSharebFSS: bFSS<uint64_t, std::bitset<1>, (1ull << KeyLength) * ValueLength> {
public:
    using BaseType = bFSS<uint64_t, std::bitset<1>, (1ull << KeyLength) * ValueLength>; 
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
        
        // nonspecified keys will evaluate to all 1s by default 
        // why? this is for convenience of encoding set membership, as nonzero means "not in set" using notation in paper
        SecretPair ret(pad, ~pad);

        // now XOR the values into pad; 
        for (const auto& [Key, Value]: data) {
            for (uint64_t currBit = 0; currBit < 1; ++currBit) {
                uint64_t offset = Key * 1 + currBit;
                ret.second[offset] = ret.first[offset] ^ Value[currBit];
            }
        }
        return ret;
    };
    // for evaluator
    TruthTable(const SecretShare& share_): BaseType(share_) {};
    Value evaluate(const Key& key) {
        Value ret;
        for (uint64_t currBit = 0; currBit < 1; ++currBit) {
            uint64_t offset = key * 1 + currBit;
            ret[currBit] = this->share[offset]; 
        }
        return ret;
    };
};

