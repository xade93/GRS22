#pragma once
#include <bits/stdc++.h>
#include <openssl/sha.h>
#include "matrix_tools.tpp"

// design choices:
// - XXXXXXXXXXXXXXXXXXXX we need boost::dynamic_bitset instead of std::bitset because OpenSSL SHA256 takes bytestream (8 bits chunk) not std::bitset (>=32 bits chunk),
//   and I want to avoid reinvent that wheel. This loses 4-8x speed on field addition ops, but should not matter.
// - KeyLength, ValueLength etc. are stored as compile-time constants instead of runtime arguments. This can work both ways as one can argue user can only select from profiles determined by experts.
//   I store as template variables s.t. std::bitset have slightly tighter memory usage bound. 
// - std::bitset is used where std::vector<bool> or std::array<bool, N> can be used. This is just my personal preference.

// I think at the end of the day, it exposes too much implementation details & minor performance issues that decision is no longer trivial.

namespace okvs {
    // wrapper for OpenSSL SHA256
    template<uint64_t I> 
    std::function<std::bitset<256>(std::bitset<I>)> SHA256Hash = [](const std::bitset<I>& input) -> std::bitset<256> {
        SHA256_CTX sha256;
        SHA256_Init(&sha256);

        // convert bitset (>= 32 bit chunks) to bytestream (8bit chunks). tail will be padded.
        // low efficiency (should be faster by iterating chunks instead) but there is platform-dependent logic to do. decide dont do it.
        // not pure function, but have to do this due to OpenSSL interface
        auto BitsetToByteStream = [](const std::bitset<I>& input, char ret[]) {
            static_assert(CHAR_BIT == 8 && I > 0);
            for (uint64_t idx = 0; idx < I; ++idx) {
                ret[idx / 8] |= ((input[idx]) << (7 - idx % 8)); 
            }
        };

        char bs[(I - 1) / 8 + 1];
        BitsetToByteStream(input, bs);

        SHA256_Update(&sha256, bs, (I - 1) / 8 + 1);
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_Final(hash, &sha256);

        // convert the 32x8 format into more elegant bitset<256>.
        std::bitset<256> ret;
        for (uint64_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) for (uint64_t offset = 0; offset < 8; ++offset) {
            ret[i * 8 + offset] = (hash[i] & (1 << offset)) != 0;
        }
        return ret;
    };
    
    // PaXoS
    // not sure how to select random engine polymorphism or template; persistent with std::function and static variable seems ugly;
    // I will stick to mt19937_64 for now. TODO change to CSPRNG
    // mt19937 is used in 2 places: 1) act as RNG source in lambda string 2) act as 'hasher'
    // also stick to SHA256 for hashing.
    template<uint64_t KeyLength, uint64_t ValueLength, uint64_t Lambda, uint64_t MaxEncodingAttempt = 10>
    struct RandomBooleanPaXoS {
    public: // TODO change back to private
        std::mt19937_64 randomEngine;
        std::unique_ptr<std::random_device> randomSource;
        static constexpr uint64_t HashedKeyLength = KeyLength + Lambda;
    public:
        using EncodedPaXoS = std::array<std::bitset<ValueLength>, HashedKeyLength>;
        using Key = std::bitset<KeyLength>;
        using Nonce = std::bitset<Lambda>;
        using Value = std::bitset<ValueLength>;
        template<typename T> using Opt = std::optional<T>;

        // why unique ptr? because I need modifiable sole ownership of the random source.
        RandomBooleanPaXoS(std::unique_ptr<std::random_device> RandomSource)
        {
            randomSource = std::move(RandomSource);
            randomEngine = std::mt19937_64((*randomSource)());
            static_assert(HashedKeyLength <= 64);
        }

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

        // securely hash binary string of length I and salt L to arbitrary length O
        // security at most 2**64, TODO improve security up to mt19937_64's space
        template<uint64_t I, uint64_t L, uint64_t O> 
        std::bitset<O> streamHash(const std::bitset<I>& input, const std::bitset<L>& salt) {
            // hash salted input via SHA256.
            // TODO fix this concat
            std::bitset<I + L> combinedInput(input.to_string() + salt.to_string()); // note this essentially "Reverse" them, due to bitset print MSB first

            std::bitset<256> hashed = SHA256Hash<I + L>(combinedInput);

            // compress the 256-bit hash into 64 bit, which is used to seed mt19937_64
            uint64_t prngSeed = 0;
            for (uint64_t i = 0; i < 256; ++i) if (hashed[i]) prngSeed ^= (1ll << (i % 64));
            std::cout << "streamHash of " << input << "+" << salt << ", yielded seed is " << prngSeed << std::endl; // TODO remove

            // finally, generate output vector.
            auto v = std::mt19937_64(prngSeed);
            auto ret = GetBitSequenceFromPRNG<O>(v);
            std::cout << "yielded hash is " << ret << std::endl;
            return ret;
        }

        // encode function optionally returns (1) the encoded vector (2) the nonce. 
        // Returns nullopt when fail to encode due to exceeding trial attempts.
        // will pad the kvs if not have sufficient entry 
        Opt<std::pair<EncodedPaXoS, Nonce>> encode(std::vector<std::pair<Key, Value>> kvs) { // intentionally need to copy kvs
            // TODO there is no need for padding if the hamming weight of v is sufficiently large. 
            // which is also the benefit of this - it depends on the number of entry used.
            // otherwise this would be so heavy.
            if (kvs.size() < KeyLength) { // run the padding process if there is insufficient key-value pairs. note random is okay here as space is exponentially larger than number of entrys
                // std::unordered_set<Key> existKeys;
                // for (auto& [k, v]: kvs) existKeys.emplace(k);

                // while (kvs.size() < KeyLength) { // TODO check for exit when n is very small
                //     auto dummyKey = GetBitSequenceFromPRNG<KeyLength>(randomEngine);
                //     if (existKeys.find(dummyKey) == existKeys.end()) {
                //         existKeys.emplace(dummyKey);
                //         auto dummyVal = GetBitSequenceFromPRNG<ValueLength>(randomEngine);
                //         kvs.emplace_back(dummyKey, dummyVal);
                //     }
                // }
            }
            
            size_t n = kvs.size(); // number of key-value pairs we wish to encode.
            using HashedKey = std::bitset<HashedKeyLength>;
            for (uint64_t trial = 0; trial <= MaxEncodingAttempt; ++trial) {
                // firstly, we generate the whole encoded matrix.
                auto nonce = Nonce();// FIXME GetBitSequenceFromPRNG<Lambda>(randomEngine);
                std::vector<HashedKey> currMatrix;
                for (auto& [key, value]: kvs) { // for each key[i], evaluate v(key[i]). note v in paper is implemented as streamHash here
                    auto encodedKey = streamHash<KeyLength, Lambda, HashedKeyLength>(key, nonce);
                    currMatrix.emplace_back(encodedKey);
                }
                assert(currMatrix.size() == kvs.size());

                // next, we check if the generated matrix is linearly independent. 
                if (isFullRank<HashedKeyLength>(currMatrix)) {
                    std::cout << "independent matrix A found: \n";
                    for (auto e: currMatrix) std::cout << e << '\n';
                    EncodedPaXoS encoded;
                    // we enter the encoding phase.
                    // for each of the columns, perform a Ax = b.
                    for (uint64_t i = 0; i < ValueLength; ++i) {
                        std::vector<bool> q(n);
                        for (uint64_t j = 0; j < n; ++j) q[j] = kvs[j].second[i]; // taking vertical slice of value portion of kvs matrix
                        auto ret_wrapper = solveLinear<HashedKeyLength>(currMatrix, q);
                        assert(ret_wrapper.has_value());
                        auto ret = ret_wrapper.value();
                        for (uint64_t j = 0; j < HashedKeyLength; ++j) encoded[j][i] = ret[j]; // copy to encode
                    }
                    std::cout << "matrix X is: \n";
                    for (auto e: encoded) std::cout << e << '\n';
                    return std::make_pair(encoded, nonce) ;
                }
            }
            return std::nullopt;
        }

        // decoding always succeed, and is equivalent to MUXing paxos by selected bits.
        Value decode(const EncodedPaXoS& encoded, const Nonce& nonce, const Key& key) {
            Value ret; 
            auto vx = streamHash<KeyLength, Lambda, HashedKeyLength>(key, nonce);
            std::cout << "decode key: " << vx << std::endl;
            for (uint64_t i = 0; i < HashedKeyLength; ++i) if (vx[i]) ret = ret ^ encoded[i];
            return ret;
        }
    };
}