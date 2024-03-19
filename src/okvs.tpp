#pragma once
#include <climits>
#include <openssl/sha.h>
#include "matrix_tools.tpp"
#include "common.tpp"

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
    
    // PaXoS using Random Boolean Matrix method (as described in "PSI from PaXoS: Fast, Malicious Private Set Intersection")
    // basically encoding is just randomly generate v until the matrix is full rank, then solve a AX=B under GF_{ValueLength} field.
    // mt19937 is used in 2 places: 1) act as RNG source in lambda string 2) act as 'hasher'
    // also stick to SHA256 for hashing.
    template<uint64_t KeyLength, uint64_t ValueLength, uint64_t Lambda, uint64_t MaxEncodingAttempt = 10>
    struct RandomBooleanPaXoS {
    protected:
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

        // securely hash binary string of length I and salt L to arbitrary length O
        // security at most 2**64, TODO improve security up to mt19937_64's space
        template<uint64_t I, uint64_t L, uint64_t O> 
        std::bitset<O> streamHash(const std::bitset<I>& input, const std::bitset<L>& salt) {
            // hash salted input via SHA256.
            std::bitset<I + L> combinedInput(input.to_string() + salt.to_string()); // note this essentially "Reverse" them, due to bitset print MSB first. not very efficient

            std::bitset<256> hashed = SHA256Hash<I + L>(combinedInput);

            // compress the 256-bit hash into 64 bit, which is used to seed mt19937_64
            uint64_t prngSeed = 0;
            for (uint64_t i = 0; i < 256; ++i) if (hashed[i]) prngSeed ^= (1ll << (i % 64));

            // finally, generate output vector.
            auto v = std::mt19937_64(prngSeed);
            auto ret = GetBitSequenceFromPRNG<O>(v);

            return ret;
        }

        // encodes the PaXoS, from key-value pairs.
        // returns (1) the encoded vector (2) the nonce, or std::nullopt if the encoding process failed.
        Opt<std::pair<EncodedPaXoS, Nonce>> encode(std::vector<std::pair<Key, Value>> kvs) { // intentionally need to copy kvs
            // there is no need for padding if the hamming weight of v is sufficiently large (which is the case since v is random, hamming weight is half of bitlength).
            size_t n = kvs.size(); // number of key-value pairs we wish to encode.
            using HashedKey = std::bitset<HashedKeyLength>;
            for (uint64_t trial = 0; trial <= MaxEncodingAttempt; ++trial) {
                // firstly, we generate the whole encoded matrix.
                auto nonce = GetBitSequenceFromPRNG<Lambda>(randomEngine);
                std::vector<HashedKey> currMatrix;
                for (auto& [key, value]: kvs) { // for each key[i], evaluate v(key[i]). note v in paper is implemented as streamHash here
                    auto encodedKey = streamHash<KeyLength, Lambda, HashedKeyLength>(key, nonce);
                    currMatrix.emplace_back(encodedKey);
                }
                assert(currMatrix.size() == kvs.size());

                // next, we check if the generated matrix is linearly independent. 
                if (isFullRank<HashedKeyLength>(currMatrix)) {
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
                    return std::make_pair(encoded, nonce) ;
                }
            }
            return std::nullopt;
        }

        // decode a PaXoS from some key.
        // decoding always succeed, and is equivalent to MUXing paxos by selected bits.
        Value decode(const EncodedPaXoS& encoded, const Nonce& nonce, const Key& key) {
            Value ret; 
            auto vx = streamHash<KeyLength, Lambda, HashedKeyLength>(key, nonce);

            for (uint64_t i = 0; i < HashedKeyLength; ++i) if (vx[i]) ret = ret ^ encoded[i];
            return ret;
        }
        
        // helper function that serialises PaXoS given into single bitset.
        // copies bit by bit, not very efficient. use with caution.
        // Layout: [ row 1 ][ row 2 ] .... [ row H ][ Nonce ]
        //         0        V         .... V*(H-1)      V*H
        Opt<std::bitset<ValueLength * HashedKeyLength + Lambda>> serialize(const Opt<std::pair<EncodedPaXoS, Nonce>>& encoded) {
            if (encoded.has_value()) {
                auto [paxos, nc] = encoded.value();
                std::bitset<ValueLength * HashedKeyLength + Lambda> ret;
                // copies paxos
                for (uint64_t i = 0; i < HashedKeyLength; ++i) {
                    for (uint64_t currBit = 0; currBit < ValueLength; ++currBit) ret[i * ValueLength + currBit] = paxos[i][currBit];
                }
                // copies tail
                for (uint64_t i = 0; i < Lambda; ++i) ret[ValueLength * HashedKeyLength + i] = nc[i];
                return ret;
            } else return std::nullopt;
        }

        // extracts EncodedPaXoS and Nonce from serialised bitstream. see serialize() for note.
        std::pair<EncodedPaXoS, Nonce> deserialize(const std::bitset<ValueLength * HashedKeyLength + Lambda>& bits) {
            EncodedPaXoS paxos;
            Nonce nc;

            // restores paxos
            for (uint64_t i = 0; i < HashedKeyLength; ++i) {
                for (uint64_t currBit = 0; currBit < ValueLength; ++currBit) paxos[i][currBit] = bits[i * ValueLength + currBit];
            }
            // restores tail
            for (uint64_t i = 0; i < Lambda; ++i) nc[i] = bits[ValueLength * HashedKeyLength + i];
            return std::make_pair(paxos, nc);
        }
    };
}