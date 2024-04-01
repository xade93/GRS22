#include <catch2/catch_test_macros.hpp>
#include "protocols/spatialhash_concat_tt.tpp"
#include "protocols/spatialhash_tt.tpp"

// tests spatialhash @ tt in 256 * 256 grid, 
TEST_CASE("soundness of spatialhash tt", "[okvs]") {
    const int bitLength = 8, Lambda = 60, L = 60, cellBitLength = 2;
    const int radius = 1 << cellBitLength;
    std::vector<std::pair<uint64_t, uint64_t>> centers;
    std::vector<std::pair<uint64_t, uint64_t>> points;
    auto genRandomPoints = [&bitLength](int n) {
        std::vector<std::pair<uint64_t, uint64_t>> ret;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> distr(0, (1 << bitLength) - 1);

        std::array<std::array<bool, 1 << bitLength>, 1 << bitLength> locations;
        int stuck = 0;
        while(stuck <= 10 && ret.size() < n) {
            int u = distr(gen), v = distr(gen);
            
            if (!locations[u][v]) {
                locations[u][v] = true;
                ret.emplace_back(u, v);
                stuck = 0;
            } else stuck++;
        }
        if (ret.size() != n) std::cout << "note: try to generate " << n << " distinct points, only managed to generate " << ret.size() << " points.\n";
        return ret;  
    };
    centers = genRandomPoints(10); // covers ~12% of area 
    points = genRandomPoints(100); // covers ~3% of area
    auto Bob = std::thread([&] {
        spatialhash_tt<bitLength, Lambda, L, cellBitLength> psi;
        psi.SetIntersectionClient(points, "localhost");
    });

    spatialhash_tt<bitLength, Lambda, L, cellBitLength> psi;
    auto intersection = psi.SetIntersectionServer(centers, "localhost", radius);

    Bob.join();

    std::cout << "Protocol execution finished. Total number of intersections: " << intersection.size() << "\n";
    
    std::set<std::pair<uint64_t, uint64_t>> groundtruth;
    for (auto [u, v]: points) if (psi.membership(centers, u, v, radius)) groundtruth.emplace(u, v);

    assert(groundtruth == intersection);
}

TEST_CASE("soundness of spatialhash concat tt", "[okvs]") {
    // 1024 * 1024 region, radius = 4
    const int bitLength = 8, Lambda = 120, L = 60, cellBitLength = 1;
    const int radius = 1 << cellBitLength;
    std::vector<std::pair<uint64_t, uint64_t>> centers;
    std::vector<std::pair<uint64_t, uint64_t>> points;
    

    // lets randomly fill centers and points, subject to the >4delta rule.
    // if can't fill n squares inside, it attemps to fill maximum possible.
    auto genRandomFarPoints = [&bitLength, &radius](int n) {
        std::vector<std::pair<uint64_t, uint64_t>> ret;
        std::random_device rd;
        std::mt19937 gen(rd());
        int boundary = (1 << bitLength) - 1;
        std::uniform_int_distribution<int> distr(0, boundary);

        std::array<std::array<bool, 1 << bitLength>, 1 << bitLength> locations;
        
        int stuck = 0;
        while(stuck <= 5 && ret.size() < n) {
            int u = distr(gen), v = distr(gen);
            bool okay = true;
            for (int x = u - 4 * radius; x <= u + 4 * radius; ++x) {
                for (int y = v - 4 * radius; y <= v + 4 * radius; ++y) {
                    if (x < 0 || y < 0 || x > boundary || y > boundary || locations[x][y]) {
                        stuck++; okay = false;
                        goto done;
                    }
                }
            }
            done:
            if (okay) {
                locations[u][v] = true;
                ret.emplace_back(u, v);
                stuck = 0;
            }
        }
        return ret;  
    };

    auto genRandomPoints = [&bitLength](int n) {
        std::vector<std::pair<uint64_t, uint64_t>> ret;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> distr(0, (1 << bitLength) - 1);

        std::array<std::array<bool, 1 << bitLength>, 1 << bitLength> locations;
        int stuck = 0;
        while(stuck <= 5 && ret.size() < n) {
            int u = distr(gen), v = distr(gen);
            
            if (!locations[u][v]) {
                locations[u][v] = true;
                ret.emplace_back(u, v);
                stuck = 0;
            } else stuck++;
        }
        return ret;  
    };

    centers = genRandomFarPoints(10);
    points = genRandomPoints(500);
    dbg(centers), dbg(points);

    auto Bob = std::thread([&] {
        spatialhash_concat_tt<bitLength, Lambda, L, cellBitLength> psi;
        psi.SetIntersectionClient(points, "localhost");
    });

    spatialhash_concat_tt<bitLength, Lambda, L, cellBitLength> psi;
    auto intersection = psi.SetIntersectionServer(centers, "localhost", radius);

    Bob.join();

    std::cout << "Protocol execution finished. Total number of intersections: " << intersection.size() << "\n";
    
    std::set<std::pair<uint64_t, uint64_t>> groundtruth;
    for (auto [u, v]: points) if (psi.membership(centers, u, v, radius)) groundtruth.emplace(u, v);

    dbg(groundtruth), dbg(intersection);

    assert(groundtruth == intersection);
}