#include <catch2/catch_test_macros.hpp>
#include "okvs.tpp"

TEST_CASE("okvs random gen looks correctly", "[okvs]") {
    auto randomSource = std::make_unique<std::random_device>();
    okvs::RandomBooleanPaXoS<6, 8, 2, 10> paxos(std::move(randomSource));
    auto ret = paxos.GetBitSequenceFromPRNG<50>(paxos.randomEngine);
    std::cout << "generated rng seq: " << ret << std::endl;
}

TEST_CASE("okvs hashing looks correctly", "[okvs]") {
    auto randomSource = std::make_unique<std::random_device>();
    auto ret = okvs::SHA256Hash<10>(std::bitset<10>("0100010101"));
    std::cout << "generated hash value: " << ret << std::endl;
}

// TEST_CASE("okvs stream hash looks correctly", "[okvs]") { // TODO uncomment; comment due to cluttering console
//     auto randomSource = std::make_unique<std::random_device>();
//     okvs::RandomBooleanPaXoS<6, 8, 2, 10> paxos(std::move(randomSource));
//     auto ret = paxos.streamHash<4, 2, 8>(std::bitset<4>("0101"), std::bitset<2>("01"));
//     std::cout << "generated stream hash value: " << ret << std::endl;
// }

TEST_CASE("reversibility of okvs encoding", "[okvs]") {
    auto randomSource = std::make_unique<std::random_device>();
    okvs::RandomBooleanPaXoS<6, 8, 2, 10> paxos(std::move(randomSource));

    using Key = std::bitset<6>;
    using Value = std::bitset<8>;

    std::vector<std::pair<Key, Value>> kvs = {{Key("111010"), Value("11010111")}, {Key("000111"), Value("00000101")}};


    std::unordered_set<Value> s; // the set of all values. this is used later for checking if value exists in the set.
    for (auto& [k, v]: kvs) s.emplace(v);

    auto encodeResult = paxos.encode(kvs);

    REQUIRE(encodeResult != std::nullopt);
    auto [encoded, nonce] = encodeResult.value();
    REQUIRE(nonce.size() != 0);
    REQUIRE(encoded.size() != 0);
    std::cout << "encoded has " << encoded.size() << " rows.\n";
    std::cout << "full matrix: \n";
    for (auto& e: encoded) std::cout << e << "\n";

    auto decodeResult = paxos.decode(encoded, nonce, Key("111010"));
    std::cout << "decoded = " << decodeResult << std::endl;

    auto decode2 = paxos.decode(encoded, nonce, Key("000111"));
    std::cout << "decoded = " << decode2 << std::endl;

    auto decode_wrong = paxos.decode(encoded, nonce, Key("111000"));
    std::cout << "decoded = " << decode_wrong << std::endl;
    REQUIRE(decodeResult == Value("11010111"));
    REQUIRE(decode2 == Value("00000101"));
    REQUIRE(s.find(decode_wrong) == s.end()); // wrong key should decode into gibberish
}