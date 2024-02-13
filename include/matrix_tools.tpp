#pragma once
#include<bits/stdc++.h>

template<size_t N>
bool isFullRank(const std::vector<std::bitset<N>>& matrix) {
    size_t m = matrix.size(); // Number of rows
    if (m == 0) return false; // Empty matrix doesn't have full rank

    size_t n = N; // Number of columns, determined by bitset size
    std::vector<std::bitset<N>> mat = matrix; // Copy of the matrix for manipulation

    size_t rank = 0;
    for (size_t col = 0; col < n; ++col) {
        // Find a row with a non-zero element in the current column
        size_t swapRow = rank;
        while (swapRow < m && !mat[swapRow][col]) {
            ++swapRow;
        }
        if (swapRow == m) continue; // No non-zero element found in this column

        // Swap the row with a non-zero element to the current row
        std::swap(mat[rank], mat[swapRow]);

        // Eliminate the current column from the subsequent rows
        for (size_t row = 0; row < m; ++row) {
            if (row != rank && mat[row][col]) {
                mat[row] ^= mat[rank]; // XOR for binary operation
            }
        }
        ++rank; // Increment rank for each pivot found
    }

    return rank == std::min(m, n); // Full rank if rank equals the smaller dimension
}

template<size_t N>
std::pair<bool, std::bitset<N>> solveSystem(const std::vector<std::bitset<N>>& A, const std::vector<bool>& b) {
    size_t m = A.size(); // Number of rows in A
    if (m == 0) return {false, {}}; // No solution if A is empty

    // Create an augmented matrix by appending b to A
    std::vector<std::bitset<N + 1>> augmentedMatrix(m);
    for (size_t i = 0; i < m; ++i) {
        for (size_t j = 0; j < N; ++j) {
            augmentedMatrix[i][j] = A[i][j];
        }
        augmentedMatrix[i][N] = b[i]; // Append b as the last column
    }

    // Gaussian elimination
    for (size_t col = 0; col < N; ++col) {
        size_t pivotRow = col;
        while (pivotRow < m && !augmentedMatrix[pivotRow][col]) {
            ++pivotRow;
        }
        if (pivotRow >= m) continue; // No pivot found, move to next column

        // Swap current row with pivot row
        std::swap(augmentedMatrix[col], augmentedMatrix[pivotRow]);

        // Eliminate other rows
        for (size_t row = 0; row < m; ++row) {
            if (row != col && augmentedMatrix[row][col]) {
                augmentedMatrix[row] ^= augmentedMatrix[col];
            }
        }
    }

    // Back substitution
    std::bitset<N> x;
    for (size_t row = 0; row < m; ++row) {
        if (augmentedMatrix[row][N]) { // If the last column is 1
            size_t pivot = augmentedMatrix[row]._Find_first();
            if (pivot == N) return {false, {}}; // Inconsistent system
            x[pivot] = 1;
        }
    }

    // Verify solution
    for (size_t i = 0; i < m; ++i) {
        bool result = 0;
        for (size_t j = 0; j < N; ++j) {
            result ^= (A[i][j] && x[j]);
        }
        if (result != b[i]) return {false, {}}; // Solution doesn't satisfy equation
    }

    return {true, x};
}
