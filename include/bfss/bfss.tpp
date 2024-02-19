#pragma once
#include <bits/stdc++.h>

// a bFSS class models a class that handles representation of a function f: Key -> Value into exactly two copies of bitstrings.
// when used as an encoder, it takes plain text description of function as key-value pairs, and return the encoded secret shares corresponding to that function.
// when used as an evaluator, it takes exactly one of the two secret shares, and point-evaluates over such secret share. 
// Usually there is some property guaranteed when evaluating over share: 
//  1. eval(share1, x) ^ eval(share2, x) == f(x)
//  2. share1 and share2 alone are almost random independently.
// But of course they are not enforced in the syntax.

template<typename Key, typename Value, int ShareLength>
class bFSS {
public:
    using SecretShare = std::bitset<ShareLength>;
    using SecretPair = std::pair<SecretShare, SecretShare>;
    // for encoder
    bFSS() = default;
    virtual ~bFSS() = default;
    virtual SecretPair encode(const std::vector<std::pair<Key, Value>>& data) = 0;
    // for evaluator
    bFSS(const SecretShare& share_): share(share_) {};
    virtual Value evaluate(const Key& key) = 0;
    SecretShare getShare() {
        return share;
    }
protected:
    SecretShare share;
};

