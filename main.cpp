#include <bits/stdc++.h>
#include "protocol.tpp"

int main() {
    std::cout << "Frontend WIP. Please run test executable instead and view code to see usage.\n";
    const int bitLength = 2, Lambda = 2, L = 30, radiusBitLength = 1;
    std::vector<std::pair<uint64_t, uint64_t>> centers = {
        {1, 1}
    };

    auto Bob = std::thread([&] {
        std::vector<std::pair<uint64_t, uint64_t>> points = {
            {1, 1}
        };
        GRS22_L_infinity_protocol::SetIntersectionClient<bitLength, Lambda, L, radiusBitLength>(points, "localhost");
    });

    auto intersection = GRS22_L_infinity_protocol::SetIntersectionServer<bitLength, Lambda, L, radiusBitLength>(centers, "localhost");

    Bob.join();

    std::cout << "Protocol execution finished. Total number of intersections: " << intersection.size() << "\n";
    for (auto e: intersection) {
        std::cout << " " << e.first << " " << e.second << '\n';
    }

    return 0;
}