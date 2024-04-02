#pragma once
#include "common.tpp"

template <size_t N>
bool isFullRank(const std::vector<std::bitset<N>>& matrix) {
    assert(matrix.size() <= N);
    std::vector<std::bitset<N>> basis(N);
    std::vector<bool> possible(N);

    int rank = 0;
    for (auto row: matrix) {
        assert(N == row.size()); // assert this is well-formed square matrix
        for (int idx = N - 1; idx >= 0; --idx) if (row[idx]) {
            if (possible[idx]) row = row ^ basis[idx];
            else {
                basis[idx] = row;
                possible[idx] = true;
                rank++;
                break;
            }
        }
    }
    return (rank == matrix.size());
}

// note this mutates A and b so cant pass reference.
// TODO courtesy from katcl
template<uint64_t M>
std::optional<std::bitset<M>> solveLinear(std::vector<std::bitset<M>> A, std::vector<bool> b) {
    assert(b.size() == A.size());
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