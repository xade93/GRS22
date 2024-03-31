#include <bits/stdc++.h>
#include "protocols/spatialhash_tt.tpp"

int main() {
    std::cout << "Below is a small example for proof-of-concept. For large-scale randomized tests and benchmarks, see tests/protocols.tpp. \n";
    const int bitLength = 4, Lambda = 20, L = 60, cellBitLength = 1;
    std::vector<std::pair<uint64_t, uint64_t>> centers = {
        {1, 1},
        {6, 6}
    };

    auto Bob = std::thread([&] {
        std::vector<std::pair<uint64_t, uint64_t>> points = {
            {1, 1},
            {1, 2},
            {0, 0},
            {1, 3},
            {4, 4},
            {6, 7},
            {6, 8}
        };
        spatialhash_tt<bitLength, Lambda, L, cellBitLength> psi;
        psi.SetIntersectionClient(points, "localhost");
    });

    spatialhash_tt<bitLength, Lambda, L, cellBitLength> psi;
    auto intersection = psi.SetIntersectionServer(centers, "localhost", 1);

    Bob.join();

    std::cout << "Protocol execution finished. Total number of intersections: " << intersection.size() << "\n";
    for (auto e: intersection) {
        std::cout << " " << e.first << " " << e.second << '\n';
    }

    return 0;
}