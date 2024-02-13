#include <catch2/catch_test_macros.hpp>
#include "matrix_tools.tpp"

TEST_CASE("matrix solver works", "[matrix_tools]") {
    std::vector<std::bitset<3>> A(2);
    A[0] = std::bitset<3>("011"), A[1] = std::bitset<3>("101");

    std::vector<bool> b = {1, 1};
    auto x = solveLinearSystem<3>(A, b);

    std::cout << x << std::endl;
    std::cout << std::endl;
}
