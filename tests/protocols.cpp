#include <catch2/catch_test_macros.hpp>
#include "protocols/spatialhash_concat_tt.tpp"
#include "protocols/spatialhash_tt.tpp"

TEST_CASE("soundness of spatialhash tt", "[protocol]") {
    const int bitLength = 10, Lambda = 200, L = 60, cellBitLength = 2;
    const int aliceCount = 20, bobCount = 10000;
    const int radius = 1 << cellBitLength;

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

    std::vector<std::pair<uint64_t, uint64_t>> centers = genRandomPoints(aliceCount);
    std::vector<std::pair<uint64_t, uint64_t>> points = genRandomPoints(bobCount);

    auto Bob = std::thread([&] {
        spatialhash_tt<bitLength, Lambda, L, cellBitLength> psi;
        psi.SetIntersectionClient(points, "localhost");
    });

    spatialhash_tt<bitLength, Lambda, L, cellBitLength> psi;
    auto intersection = psi.SetIntersectionServer(centers, "localhost", radius);

    Bob.join();

    std::cout << "Protocol execution finished. Total number of intersections: " << intersection.size() << std::endl;
    
    std::set<std::pair<uint64_t, uint64_t>> groundtruth;
    for (auto [u, v]: points) if (psi.membership(centers, u, v, radius)) groundtruth.emplace(u, v);

    std::cout << "Groundtruth: " << groundtruth.size() << std::endl;
    dbg(groundtruth), dbg(intersection);
    REQUIRE(groundtruth == intersection);
}

TEST_CASE("soundness of spatialhash concat tt", "[protocol]") {
    // 1024 * 1024 region, radius = 4
    const int bitLength = 11, Lambda = 200, L = 60, cellBitLength = 4;
    const int aliceCount = 5, bobCount = 10000;
    const int radius = 1 << cellBitLength;
    
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

    std::vector<std::pair<uint64_t, uint64_t>> centers;
    std::vector<std::pair<uint64_t, uint64_t>> points;

    centers = genRandomFarPoints(aliceCount);
    points = genRandomPoints(bobCount);
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

    REQUIRE(groundtruth == intersection);
}