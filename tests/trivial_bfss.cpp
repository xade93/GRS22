#include <catch2/catch_test_macros.hpp>
#include "bfss/trivial_bfss.tpp" 

TEST_CASE("Trivial bFSS Soundness", "[trivialbfss]") {
    TruthTable<3, 2> tt;
    std::vector<std::pair<uint64_t, std::bitset<2>>> vec = {{3, std::bitset<2>(01)}, {2, std::bitset<2>(10)}};
    auto ret = tt.encode(vec);

    TruthTable<3, 2> decoder1(ret.first);
    TruthTable<3, 2> decoder2(ret.second);

    auto val = decoder1.evaluate(2) ^ decoder2.evaluate(2);
    REQUIRE(val == std::bitset<2>(10));

    auto val2 = decoder1.evaluate(3) ^ decoder2.evaluate(3);
    REQUIRE(val2 == std::bitset<2>(01));
}
