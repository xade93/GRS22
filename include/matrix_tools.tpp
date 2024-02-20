#pragma once
#include <bits/stdc++.h>

template <size_t N>
bool isFullRank(const std::vector<std::bitset<N>>& matrix) {
    size_t M = matrix.size();  // Number of rows
    if (M == 0) return false;  // Empty matrix doesn't have full rank

    size_t n = N;  // Number of columns, determined by bitset size
    std::vector<std::bitset<N>> mat = matrix;  // Copy of the matrix for manipulation

    size_t rank = 0;
    for (size_t col = 0; col < n; ++col) {
        // Find a row with a non-zero element in the current column
        size_t swapRow = rank;
        while (swapRow < M && !mat[swapRow][col]) ++swapRow;
    
        if (swapRow == M) continue;  // No non-zero element found in this column

        // Swap the row with a non-zero element to the current row
        std::swap(mat[rank], mat[swapRow]);

        // Eliminate the current column from the subsequent rows
        for (size_t row = 0; row < M; ++row) 
            if (row != rank && mat[row][col]) mat[row] ^= mat[rank];  // XOR for binary operation
        
        ++rank;  // Increment rank for each pivot found
    }

    return rank == std::min(M, n);  // Full rank if rank equals the smaller dimension
}

// note this mutates A and b so cant pass reference.
// TODO courtesy from katcl
template<uint64_t M>
std::optional<std::bitset<M>> solveLinear(std::vector<std::bitset<M>> A, std::vector<bool> b) {
    uint64_t n = A.size(), rank = 0, br;
    std::vector<uint64_t> col(M);
    std::iota(col.begin(), col.end(), 0);
    for (uint64_t i = 0; i < n; ++i) {
        for (br = i; br < n; ++br)
            if (A[br].any()) break;
        if (br == n) {
            for (uint64_t j = i; j < n; ++j) if (b[j]) return std::nullopt;
            break;
        }
        int bc = (int)A[br]._Find_next(i - 1);
        std::swap(A[i], A[br]);
        auto tmp = b[i]; b[i] = b[br], b[br] = tmp; // swap b[i] and b[br]
        std::swap(col[i], col[bc]);
        for(uint64_t j = 0; j < n; ++j) if (A[j][i] != A[j][bc]) {
            A[j].flip(i);
            A[j].flip(bc);
        }
        for(uint64_t j = i + 1; j < n; ++j) if (A[j][i]) {
            b[j] = b[j] ^ b[i]; // no ^= operator for vector<bool> ...
            A[j] = A[j] ^ A[i];
        }
        rank++;
    }
    std::bitset<M> x;
    for (uint64_t i = rank; i--;) {
        if (!b[i]) continue;
        x[col[i]] = 1;
        for(uint64_t j = 0; j < i; ++j) b[j] = b[j] ^ A[j][i]; // again
    }
    return x;
}