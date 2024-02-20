#include <catch2/catch_test_macros.hpp>
#include "bfss/spatial_hash.tpp" 

TEST_CASE("Spatial Hash Soundness", "[spatialhash]") {
    SpatialHash<6, 3, 1> spatial_hasher;
    spatial_hasher.insert(12, 21, std::bitset<3>(5));

    auto ret = spatial_hasher.encode();
    REQUIRE(ret != std::nullopt);

    auto [paxos, nonce] = ret.value();
    auto ret2 = spatial_hasher.decode(paxos, nonce, 12, 21);

    REQUIRE(ret2 == std::bitset<3>(5));
}
