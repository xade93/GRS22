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
