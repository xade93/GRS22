#include <catch2/catch_test_macros.hpp>
#include "matrix_tools.tpp" 

TEST_CASE("Solve linear system over binary field", "[solveLinear]") {
    std::vector<std::bitset<3>> A = {
        std::bitset<3>("110"), 
        std::bitset<3>("101")  
    };

    std::vector<bool> b = {true, true}; 

    auto solution = solveLinear<3>(A, b);

    std::bitset<3> expectedSolution("100"); 

    REQUIRE(solution.has_value());
    auto val = solution.value();

    std::cout << "Computed solution: " << val << std::endl;
}
