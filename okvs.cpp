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
        auto BitsetToByteStream = [&I](const std::bitset<I>& input) {
            static_assert(CHAR_BIT == 8 && I > 0);
            char ret[(I - 1) / 8 + 1]; 
            for (uint64_t idx = 0; idx < I; ++idx) {
                ret[idx / 8] |= ((input[idx]) << (7 - idx % 8)); 
            }
            return ret;
        };

        auto bs = BitsetToByteStream(input);

        SHA256_Update(&sha256, input, bs, (I - 1) / 8 + 1);
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
        constexpr HashedKeyLength = KeyLength + Lambda;
        using D = std::array<std::bitset<ValueLength>, HashedKeyLength>;
        using N = std::bitset<Lambda>; // a nonce is always a bitstring of length Lambda.
        template<typename T> using Opt = std::optional<T>;

        RandomBooleanPaXoS(const std::random_device& RandomSource): 
            randomEngine(std::mt19937_64(RandomSource)), hash(Hash64()) {
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
                for (uint64_t currBit = idx; currBit < min(idx + 64, L); ++currBit) {
                    bits[currBit] = currBatch[currBit - idx];
                }
            }
            return bits;
        }

        // encode function optionally returns (1) the encoded vector (2) the nonce. 
        // Returns nullopt when fail to encode due to exceeding trial attempts.
        Opt<std::pair<D, N>> encode(const std::vector<std::pair<uint64_t, uint64_t>>& kvs) { // TODO support for longer kvs
            for (uint64_t trial = 0; trial <= MaxEncodingAttempt; ++trial) {
                std::bitset<Lambda> nonce = GetBitSequenceFromPRNG<Lambda>(randomEngine);
                auto hash = SHA256Hash<KeyLength, Lambda>()
            }
            return std::nullopt;
        }
    private:
        std::mt19937_64 randomEngine;
        Hash64 hash;
    };
}