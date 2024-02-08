#include <bits/stdc++.h>
#include <openssl/sha.h>

// design choices:
// - XXXXXXXXXXXXXXXXXXXX we need boost::dynamic_bitset instead of std::bitset because OpenSSL SHA256 takes bytestream (8 bits chunk) not std::bitset (>=32 bits chunk),
//   and I want to avoid reinvent that wheel. This loses 4-8x speed on field addition ops, but should not matter.
// - KeyLength, ValueLength etc. are stored as compile-time constants instead of runtime arguments. This can work both ways as one can argue user can only select from profiles determined by experts.
//   I store as template variables s.t. std::bitset have slightly tighter memory usage bound. 
// - std::bitset is used where std::vector<bool> or std::array<bool, N> can be used. This is just my personal preference.

// I think at the end of the day, it exposes too much implementation details & minor performance issues that decision is no longer trivial.


namespace okvs {
    // wrapper for OpenSSL SHA256
    template<uint64_t I, uint64_t N> 
    std::function<std::bitset<256>(std::bitset<I>, std::bitset<N>)> SHA256Hash = [](const std::bitset<I>& input, const std::bitset<N>& nonce) -> std::bitset<256> {
        SHA256_CTX sha256;
        SHA256_Init(&sha256);

        // convert bitset (>= 32 bit chunks) to bytestream (8bit chunks). tail will be padded.
        // low efficiency (should be faster by iterating chunks instead) but there is platform-dependent logic to do. decide dont do it.
        // not functional, but have to do this due to OpenSSL interface
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
        static constexpr uint64_t HashedKeyLength = KeyLength + Lambda;
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
                std::bitset<64> currBatch = src();
                for (uint64_t currBit = idx; currBit < std::min(idx + 64, L); ++currBit) {
                    bits[currBit] = currBatch[currBit - idx];
                }
            }
            return bits;
        }

        // encode function optionally returns (1) the encoded vector (2) the nonce. 
        // Returns nullopt when fail to encode due to exceeding trial attempts.
        Opt<std::pair<EncodedPaXoS, Nonce>> encode(const std::vector<std::pair<Key, Value>>& kvs) { // TODO support for longer kvs
            using HashedKey = std::bitset<HashedKeyLength>;
            for (uint64_t trial = 0; trial <= MaxEncodingAttempt; ++trial) {
                auto nonce = GetBitSequenceFromPRNG<Lambda>(randomEngine);
                std::vector<HashedKey> currMatrix;
                for (auto& [key, value]: kvs) { // for each key[i], evaluate v(key[i]). Now we wish to sanity-check if the resulting matrix is linearly independent.
                    std::bitset<256> hashed = SHA256Hash<KeyLength, Lambda>(key, nonce);

                    // next, compress the 256-bit hash into 64 bit
                    uint64_t prngSeed = 0;
                    for (uint64_t i = 0; i < 256; ++i) if (hashed[i]) prngSeed ^= (1ll << (i % 64));

                    // finally, generate output vector.
                    auto v = std::mt19937_64(prngSeed);
                    auto encodedKey = GetBitSequenceFromPRNG<HashedKeyLength>(v);
                    currMatrix.emplace_back(encodedKey);
                }
                assert(currMatrix.size() == kvs.size());
            }
            return std::nullopt;
        }
    private:
        std::mt19937_64 randomEngine;
        std::unique_ptr<std::random_device> randomSource;
    };
}