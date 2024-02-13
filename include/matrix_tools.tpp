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

template <size_t M>
std::bitset<M> solveLinearSystem(std::vector<std::bitset<M>>& A, std::vector<bool>& b) {
    int N = A.size();
    std::bitset<M> x;
    for (size_t col = 0, row = 0; col < M && row < N; ++col) {
        size_t sel = row;
        for (size_t i = row; i < N; ++i) {
            if (A[i][col]) {
                sel = i;
                break;
            }
        }
        if (!A[sel][col]) continue; // No pivot in this column, skip it.
        std::swap(A[sel], A[row]); // Swap rows.
        if (b[sel] != b[row]) {
            bool temp = b[sel];
            b[sel] = b[row];
            b[row] = temp;
        }


        for (size_t i = 0; i < N; ++i) {
            if (i != row && A[i][col]) {
                A[i] ^= A[row]; // Add (XOR) the rows to eliminate column 'col'.
                b[i] = b[i] != b[row];
            }
        }
        ++row;
    }

    // Back substitution (optional here since we work in F2 and assume at least one solution exists).
    for (size_t i = 0; i < N; ++i) {
        if (b[i]) {
            for (size_t j = 0; j < M; ++j) {
                if (A[i][j]) {
                    x[j] = b[i];
                    break;
                }
            }
        }
    }

    return x; // x is a solution to Ax = b
}