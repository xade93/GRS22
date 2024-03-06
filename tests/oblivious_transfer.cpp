// modified from libOTe TODO courtesy
#include <catch2/catch_test_macros.hpp>
#include "common.tpp"
#include "oblivious_transfer.tpp"
#include <libOTe/TwoChooseOne/Iknp/IknpOtExtReceiver.h>
#include <libOTe/TwoChooseOne/Iknp/IknpOtExtSender.h>

TEST_CASE("AES encryption/decryption and serialisation of long message", "[cryptoTools]") {
    block key(12345, 67890);
    // generate a random sequence of 4 * 128 - 1 length, so that we can test a sufficient long example that also reveal problem in endianness / not divisible case
    
    std::random_device dev; std::mt19937_64 rng(dev());
    std::bitset<511> bs = GetBitSequenceFromPRNG<511>(rng);
    std::cout << "generated random sequence: " << bs << std::endl;

    auto enc = aesEncrypt<4>(conversion_tools::bsToBlocks<511>(bs), key);
    auto dec = conversion_tools::blocksToBs<511>(aesDecrypt<4>(enc, key));
    
    REQUIRE(dec == bs);
}

TEST_CASE("Oblivious Transfer Interface (long message)", "[libOTe]") {
    const int n = 4;
    std::string ip = "localhost";
    const int bitLength = 64;
    using Element = std::bitset<bitLength>;
    std::array<std::pair<Element, Element>, n> content = {{
        {Element(114514), Element(1919810)},
        {Element(12), Element(16)},
        {Element(1ull << 4), Element(1ull << 15)},
        {Element(0), Element(1)}
    }};

    auto thrd = std::thread([&] {
        CHECK_NOTHROW(TwoChooseOne_Sender<IknpOtExtSender, IknpOtExtReceiver, bitLength, n>(ip, content));
    });

    std::bitset<n> choice(0b1010);
    std::array<Element, n> ret;
    CHECK_NOTHROW(ret = TwoChooseOne_Receiver<IknpOtExtSender, IknpOtExtReceiver, bitLength, n>(ip, choice));
    thrd.join();

    for (uint64_t idx = 0; idx < n; ++idx) {
        std::cout << "oblivious transfer returned " << ret[idx] << std::endl;
        REQUIRE(ret[idx] == (choice[idx] ? content[idx].second : content[idx].first));
    }
}