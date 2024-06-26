#include <catch2/catch_test_macros.hpp>
#include "common.tpp"
#include "okvs.tpp"

TEST_CASE("okvs hashing looks correctly", "[okvs]") {
    auto randomSource = std::make_unique<std::random_device>();
    auto ret = okvs::SHA256Hash<10>(std::bitset<10>("0100010101"));
    std::cout << "generated hash value: " << ret << std::endl;
}

TEST_CASE("okvs stream hash looks correctly", "[okvs]") { // TODO uncomment; comment due to cluttering console
    const size_t strLen = 100, saltLen = 80;
    std::random_device rd;
    std::mt19937_64 gen(rd());
    auto x = GetBitSequenceFromPRNG<strLen>(gen);
    auto s = GetBitSequenceFromPRNG<saltLen>(gen);
    std::cout << "WTF" << std::endl;
    dbg(x, s);
    auto randomSource = std::make_unique<std::random_device>();
    okvs::RandomBooleanPaXoS<0, 0, 0, 0> paxos(std::move(randomSource));
    auto ret = paxos.streamHash<strLen, saltLen, 15>(x, s);
    
    auto randomSource2 = std::make_unique<std::random_device>();
    okvs::RandomBooleanPaXoS<0, 0, 0, 0> paxos2(std::move(randomSource2));
    auto ret2 = paxos2.streamHash<strLen, saltLen, 15>(x, s);
    
    REQUIRE(ret == ret2);
}

TEST_CASE("reversibility of okvs encoding (large case)", "[okvs]") {
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
    // REQUIRE(s.find(decode_wrong) == s.end()); // wrong key should decode into gibberish
}

TEST_CASE("reversibility of okvs encoding (small case)", "[okvs]") {
    auto randomSource = std::make_unique<std::random_device>();
    okvs::RandomBooleanPaXoS<3, 2, 0, 10> paxos(std::move(randomSource));

    using Key = std::bitset<3>;
    using Value = std::bitset<2>;

    std::vector<std::pair<Key, Value>> kvs = {{Key("101"), Value("01")}, {Key("000"), Value("10")}};


    std::unordered_set<Value> s; // the set of all values. this is used later for checking if value exists in the set.
    for (auto& [k, v]: kvs) s.emplace(v);

    auto encodeResult = paxos.encode(kvs);

    REQUIRE(encodeResult != std::nullopt);
    auto [encoded, nonce] = encodeResult.value();
    REQUIRE(encoded.size() != 0);
    std::cout << "encoded has " << encoded.size() << " rows.\n";
    std::cout << "full matrix: \n";
    for (auto& e: encoded) std::cout << e << "\n";

    auto decodeResult = paxos.decode(encoded, nonce, Key("101"));
    std::cout << "decoded = " << decodeResult << std::endl;

    auto decode2 = paxos.decode(encoded, nonce, Key("000"));
    std::cout << "decoded = " << decode2 << std::endl;

    auto decode_wrong = paxos.decode(encoded, nonce, Key("111"));
    std::cout << "decoded = " << decode_wrong << std::endl;
    REQUIRE(decodeResult == Value("01"));
    REQUIRE(decode2 == Value("10"));
    // WARN(s.find(decode_wrong) != s.end()); // wrong key should decode into gibberish
}