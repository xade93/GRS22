#pragma once
#include "common.tpp"
#include <eigen3/Eigen/Dense>

bits bitwiseXor(const bits& v1, const bits& v2) {
    assert(v1.size() == v2.size());
    bits ret(v1);
    for (size_t i = 0; i < v2.size(); ++i) ret[i] = ret[i] ^ v2[i];
    return ret;
}

void bitwiseXorOverwrite(bits& v1, const bits& v2) {
    assert(v1.size() == v2.size());
    for (size_t i = 0; i < v2.size(); ++i) v1[i] = v1[i] ^ v2[i];
}


// given a binary square matrix, it checks if its rank is equal to the number of rows.
// assert fail if not well-formed square matrix.
// this is the quite famous "linear basis" algorithm. reference: https://ouuan.github.io/post/%E7%BA%BF%E6%80%A7%E5%9F%BA%E5%AD%A6%E4%B9%A0%E7%AC%94%E8%AE%B0/
bool isFullRank(const std::vector<bits>& matrix) {
    const size_t N = matrix.size();
    std::vector<bits> basis(N);
    bits possible(N);

    int rank = 0;
    for (auto row: matrix) {
        assert(N == row.size()); // assert this is well-formed square matrix
        for (int idx = N - 1; idx >= 0; --idx) if (row[idx]) {
            if (possible[idx]) bitwiseXorOverwrite(row, basis[idx]), rank++;
            else {
                basis[idx] = row;
                possible[idx] = true;
                break;
            }
        }
    }
    return (rank == matrix.size());
}

// note this mutates A and b so cant pass reference.
// TODO courtesy from katcl
std::optional<bits> solveLinear(std::vector<bits> A, bits b) {
    return std::nullopt; // TODO
}