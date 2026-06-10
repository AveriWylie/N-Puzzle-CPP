#pragma once
#include <cstdint>
#include <cstddef>
#include <functional>

/*
---------------------------------------------------------------------------
u256: fixed 256-bit unsigned integer
---------------------------------------------------------------------------
Stands in for Python's arbitrary-width int. The packed board needs
n*n * bits bits, where bits = (n*n-1).bit_length(): 64 at n=4, 125 at n=5,
216 at n=6. A single machine word cannot hold n >= 5, so the packed state
is held in four uint64 words (least significant word first). Only the
operations the packing logic uses are implemented: <<, >>, &, |, ~, ==, <,
and a low-word read for extracting small tile values.
---------------------------------------------------------------------------
*/
struct u256 {
    uint64_t w[4];

    u256() : w{0, 0, 0, 0} {}
    explicit u256(uint64_t v) : w{v, 0, 0, 0} {}

    // logical left shift by k bits
    u256 operator<<(unsigned k) const {
        u256 r;
        if (k >= 256) return r;
        unsigned ws = k / 64, bs = k % 64;
        for (int i = 3; i >= 0; --i) {
            uint64_t lo = 0, hi = 0;
            int s = i - (int)ws;
            if (s >= 0 && s < 4) lo = w[s] << bs;
            if (bs != 0 && s - 1 >= 0 && s - 1 < 4) hi = w[s - 1] >> (64 - bs);
            r.w[i] = lo | hi;
        }
        return r;
    }

    // logical right shift by k bits
    u256 operator>>(unsigned k) const {
        u256 r;
        if (k >= 256) return r;
        unsigned ws = k / 64, bs = k % 64;
        for (int i = 0; i < 4; ++i) {
            uint64_t lo = 0, hi = 0;
            int s = i + (int)ws;
            if (s < 4) lo = w[s] >> bs;
            if (bs != 0 && s + 1 < 4) hi = w[s + 1] << (64 - bs);
            r.w[i] = lo | hi;
        }
        return r;
    }

    u256 operator&(const u256& o) const {
        u256 r;
        for (int i = 0; i < 4; ++i) r.w[i] = w[i] & o.w[i];
        return r;
    }

    u256 operator|(const u256& o) const {
        u256 r;
        for (int i = 0; i < 4; ++i) r.w[i] = w[i] | o.w[i];
        return r;
    }

    u256 operator~() const {
        u256 r;
        for (int i = 0; i < 4; ++i) r.w[i] = ~w[i];
        return r;
    }

    bool operator==(const u256& o) const {
        return w[0] == o.w[0] && w[1] == o.w[1] && w[2] == o.w[2] && w[3] == o.w[3];
    }
    bool operator!=(const u256& o) const { return !(*this == o); }

    // lexicographic, most significant word first (matches int ordering)
    bool operator<(const u256& o) const {
        for (int i = 3; i >= 0; --i) {
            if (w[i] != o.w[i]) return w[i] < o.w[i];
        }
        return false;
    }

    // low 64 bits: used after (packed >> k) & mask, where the result is a
    // single tile value that always fits in one word
    uint64_t low() const { return w[0]; }
};

struct u256_hash {
    size_t operator()(const u256& x) const {
        // mix the four words (splitmix64-style finaliser per word, xored)
        uint64_t h = 0;
        for (int i = 0; i < 4; ++i) {
            uint64_t z = x.w[i] + 0x9e3779b97f4a7c15ull;
            z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
            z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
            z = z ^ (z >> 31);
            h ^= z + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
        }
        return (size_t)h;
    }
};
