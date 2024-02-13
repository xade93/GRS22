#include <catch2/catch_test_macros.hpp>
#include "okvs.tpp"

TEST_CASE("OKVS works correctly", "[okvs]") {
    auto randomSource = std::make_unique<std::random_device>();
    okvs::RandomBooleanPaXoS<6, 8, 2, 10> paxos(std::move(randomSource));

    using Key = std::bitset<6>;
    using Value = std::bitset<8>;

    std::vector<std::pair<Key, Value>> kvs = {{Key("011000"), Value("01010101")}};

    auto ret = paxos.encode(kvs);

    REQUIRE(ret != std::nullopt);
    auto [encoded, nonce] = ret.value();

    REQUIRE(nonce.size() != 0);
    REQUIRE(encoded.size() != 0);
}