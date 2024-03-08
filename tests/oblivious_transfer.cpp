// modified from libOTe TODO courtesy
#include <catch2/catch_test_macros.hpp>
#include "common.tpp"
#include "oblivious_transfer.tpp"
#include <libOTe/TwoChooseOne/Iknp/IknpOtExtReceiver.h>
#include <libOTe/TwoChooseOne/Iknp/IknpOtExtSender.h>

TEST_CASE("AES encryption/decryption and serialisation of long message", "[cryptoTools]") {
    const int messageLength = 1025;
    const int packetCount = (messageLength + 127) / 128;

    block key(12345, 67890);
    // generate a random sequence of 1025 messageLength, so that we can test a sufficient long example that also reveal problem in endianness / not divisible case
    
    std::random_device dev; std::mt19937_64 rng(dev());
    std::bitset<messageLength> bs = GetBitSequenceFromPRNG<messageLength>(rng);
    // std::cout << "generated random sequence: " << bs << std::endl;

    auto enc = aesEncrypt<packetCount>(conversion_tools::bsToBlocks<messageLength>(bs), key);
    auto dec = conversion_tools::blocksToBs<messageLength>(aesDecrypt<packetCount>(enc, key));
    
    REQUIRE(dec == bs);

    block key2(12345, 67891);
    auto dec2 = conversion_tools::blocksToBs<messageLength>(aesDecrypt<packetCount>(enc, key2));
    REQUIRE(dec2 != bs);
}

TEST_CASE("Oblivious Transfer Interface (long message)", "[libOTe]") {
    const int n = 5;
    const int bitLength = 1025;

    std::random_device dev; std::mt19937_64 rng(dev());
    std::string ip = "localhost";
    
    using Element = std::bitset<bitLength>;

    auto randElem = [&]() {return GetBitSequenceFromPRNG<bitLength>(rng); };

    // init an random array
    std::array<std::pair<Element, Element>, n> content;
    for (auto& [u, v]: content) u = randElem(), v = randElem();

    auto thrd = std::thread([&] {
        CHECK_NOTHROW(TwoChooseOne_Sender<IknpOtExtSender, IknpOtExtReceiver, bitLength, n>(ip, content));
    });

    std::bitset<n> choice = GetBitSequenceFromPRNG<n>(rng);
    std::array<Element, n> ret;
    CHECK_NOTHROW(ret = TwoChooseOne_Receiver<IknpOtExtSender, IknpOtExtReceiver, bitLength, n>(ip, choice));
    thrd.join();

    for (uint64_t idx = 0; idx < n; ++idx) {
        std::cout << "oblivious transfer returned " << ret[idx] << std::endl;
        REQUIRE(ret[idx] == (choice[idx] ? content[idx].second : content[idx].first));
    }
}