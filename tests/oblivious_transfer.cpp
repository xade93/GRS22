// modified from libOTe TODO courtesy
#include <catch2/catch_test_macros.hpp>
#include "oblivious_transfer.tpp"

TEST_CASE("conversion utility correctness", "[libOTe]") {
    std::bitset<5> bs(0b01010);
    auto ret = conversion_tools::bsToBlock<5>(bs);
    auto r2 = conversion_tools::blockToBs<5>(ret);
    REQUIRE(r2 == bs);
}

TEST_CASE("Oblivious Transfer Interface", "[libOTe]") {
    const int n = 4;
    std::string ip = "localhost:1212";
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