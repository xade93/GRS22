#include <catch2/catch_test_macros.hpp>
#include "protocols/spatialhash_concat_tt.tpp"
#include "protocols/spatialhash_tt.tpp"

TEST_CASE("soundness of spatialhash tt", "[okvs]") {
    const int bitLength = 4, Lambda = 20, L = 60, cellBitLength = 1;
    std::vector<std::pair<uint64_t, uint64_t>> centers = {
        {1, 1},
        {6, 6}
    };
    std::vector<std::pair<uint64_t, uint64_t>> points = {
            {1, 1},
            {1, 2},
            {0, 0},
            {1, 3},
            {4, 4},
            {6, 7},
            {6, 8}
        };
    auto Bob = std::thread([&] {
        spatialhash_tt<bitLength, Lambda, L, cellBitLength> psi;
        psi.SetIntersectionClient(points, "localhost");
    });

    spatialhash_tt<bitLength, Lambda, L, cellBitLength> psi;
    auto intersection = psi.SetIntersectionServer(centers, "localhost", 1);

    Bob.join();

    std::cout << "Protocol execution finished. Total number of intersections: " << intersection.size() << "\n";
    
    std::set<std::pair<uint64_t, uint64_t>> groundtruth;
    for (auto [u, v]: points) if (psi.membership(centers, u, v, 1)) groundtruth.emplace(u, v);

    assert(groundtruth == intersection);
}

TEST_CASE("soundness of spatialhash concat tt", "[okvs]") {
    const int bitLength = 4, Lambda = 20, L = 60, cellBitLength = 1;
    std::vector<std::pair<uint64_t, uint64_t>> centers = {
        {1, 1},
        {6, 6}
    };
    std::vector<std::pair<uint64_t, uint64_t>> points = {
            {1, 1},
            {1, 2},
            {0, 0},
            {1, 3},
            {4, 4},
            {6, 7},
            {6, 8}
        };
    auto Bob = std::thread([&] {
        spatialhash_concat_tt<bitLength, Lambda, L, cellBitLength> psi;
        psi.SetIntersectionClient(points, "localhost");
    });

    spatialhash_concat_tt<bitLength, Lambda, L, cellBitLength> psi;
    auto intersection = psi.SetIntersectionServer(centers, "localhost", 1);

    Bob.join();

    std::cout << "Protocol execution finished. Total number of intersections: " << intersection.size() << "\n";
    
    std::set<std::pair<uint64_t, uint64_t>> groundtruth;
    for (auto [u, v]: points) if (psi.membership(centers, u, v, 1)) groundtruth.emplace(u, v);

    dbg(groundtruth), dbg(intersection);

    assert(groundtruth == intersection);
}