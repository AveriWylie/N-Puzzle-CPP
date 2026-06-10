#pragma once
#include "u256.hpp"
#include <array>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <cstdio>
#include <cmath>

// Boards are held in a fixed array sized to the largest supported board
// (6x6 -> 36 cells). Only the first n*n entries are used for a given n,
// mirroring Python's variable-length tuple.
constexpr int MAXCELLS = 36;

using Board = std::array<int, MAXCELLS>;
using Path = std::vector<Board>;

inline std::mt19937& rng() {
    static std::mt19937 g(std::random_device{}());
    return g;
}

// number of bits needed to represent x (Python int.bit_length)
inline int bit_length(uint64_t x) {
    return x == 0 ? 0 : (64 - __builtin_clzll(x));
}


/*
---------------------------------------------------------------------------
Nibble-packed board (internal representation)
---------------------------------------------------------------------------
*/

inline u256 pack(const Board& state, int size) {

    int bits = bit_length((uint64_t)(size - 1));
    u256 result;

    for (int i = 0; i < size; ++i) {
        result = result | (u256((uint64_t)state[i]) << (unsigned)(i * bits));
    }

    return result;
}

inline Board unpack(const u256& packed, int n) {

    int bits = bit_length((uint64_t)(n * n - 1));
    u256 mask((uint64_t)((1u << bits) - 1));
    Board out{};

    for (int i = 0; i < n * n; ++i) {
        out[i] = (int)((packed >> (unsigned)(i * bits)) & mask).low();
    }

    return out;
}

inline u256 pack_swap(const u256& packed, int i, int j, int bits) {

    u256 mask((uint64_t)((1u << bits) - 1));
    unsigned bi = (unsigned)(i * bits), bj = (unsigned)(j * bits);
    uint64_t vi = ((packed >> bi) & mask).low();
    uint64_t vj = ((packed >> bj) & mask).low();

    return (packed & ~((mask << bi) | (mask << bj))) | (u256(vj) << bi) | (u256(vi) << bj);
}

inline int packed_blank(const u256& packed, int size, int bits) {

    u256 mask((uint64_t)((1u << bits) - 1));

    for (int i = 0; i < size; ++i) {
        if (((packed >> (unsigned)(i * bits)) & mask).low() == 0) return i;
    }

    throw std::runtime_error("board contains no blank tile");
}

// ---------------------------------------------------------------------------


/*
---------------------------------------------------------------------------
State representation helpers
---------------------------------------------------------------------------
*/

inline Board make_goal(int n) {
    Board g{};
    int size = n * n;
    for (int i = 0; i < size - 1; ++i) g[i] = i + 1;
    g[size - 1] = 0;
    return g;
}

inline Board random_state(int n) {
    int size = n * n;
    Board state{};
    for (int i = 0; i < size; ++i) state[i] = i;
    std::shuffle(state.begin(), state.begin() + size, rng());
    return state;
}

inline std::vector<Board> get_neighbors(const Board& state, int n);  // fwd

// EASY_MOVES = {4: 30, 5: 22, 6: 18}
// just to easily solve higher order n configurations, linear time in this problem very
// easily exponentiates
inline Board easy_state(int n) {

    int moves;
    if (n == 4) moves = 30;
    else if (n == 5) moves = 22;
    else if (n == 6) moves = 18;
    else moves = 4 * n;

    Board state = make_goal(n);
    bool have_prev = false;
    Board prev{};

    for (int m = 0; m < moves; ++m) {
        std::vector<Board> all = get_neighbors(state, n);
        std::vector<Board> options;

        for (auto& s : all) {
            if (!(have_prev && s == prev)) options.push_back(s);
        }

        prev = state;
        have_prev = true;
        std::uniform_int_distribution<size_t> pick(0, options.size() - 1);
        state = options[pick(rng())];
    }
    return state;
}

inline std::pair<int, int> find_blank(const Board& state, int n) {
    int i = (int)(std::find(state.begin(), state.begin() + n * n, 0) - state.begin());
    return {i / n, i % n};
}

inline Board swap_tiles(const Board& state, int i, int j) {
    Board lst = state;
    std::swap(lst[i], lst[j]);
    return lst;
}

// ---------------------------------------------------------------------------


/*
---------------------------------------------------------------------------
Move generation
---------------------------------------------------------------------------
*/

inline std::vector<Board> get_neighbors(const Board& state, int n) {
    auto [row, col] = find_blank(state, n);
    int blank = row * n + col;
    std::vector<Board> result;

    static const int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (auto& d : dirs) {
        int nr = row + d[0], nc = col + d[1];
        if (0 <= nr && nr < n && 0 <= nc && nc < n) {
            result.push_back(swap_tiles(state, blank, nr * n + nc));
        }
    }
    return result;
}

// ---------------------------------------------------------------------------

/*
---------------------------------------------------------------------------
Heuristics
---------------------------------------------------------------------------
*/

inline int manhattan_distance(const Board& state, int n) {

    int total = 0;

    for (int i = 0; i < n * n; ++i) {

        int v = state[i];

        if (v) {
            total += std::abs(i / n - (v - 1) / n) + std::abs(i % n - (v - 1) % n);
        }

    }
    return total;
}

// ---------------------------------------------------------------------------


inline bool is_solvable(const Board& state, int n) {
    std::vector<int> tiles;
    for (int i = 0; i < n * n; ++i)
        if (state[i]) tiles.push_back(state[i]);

    long inversions = 0;
    for (size_t i = 0; i < tiles.size(); ++i)
        for (size_t j = i + 1; j < tiles.size(); ++j)
            if (tiles[i] > tiles[j]) ++inversions;

    if (n % 2 == 1)  // odd-width grid
        return inversions % 2 == 0;

    int blank_idx = (int)(std::find(state.begin(), state.begin() + n * n, 0) - state.begin());
    int blank_row_from_bottom = n - blank_idx / n;  // even-width grid
    return (inversions + blank_row_from_bottom) % 2 == 1;
}


struct HeapItem {
    int f, g;
    u256 p;
};

struct HeapCmp {
    bool operator()(const HeapItem& a, const HeapItem& b) const {
        if (a.f != b.f) return a.f > b.f;
        if (a.g != b.g) return a.g > b.g;
        return b.p < a.p;  // (f, g, packed) ascending -> min on top
    }
};

inline bool astar(const Board& start, int n, Path& out_path, int (*heuristic)(const Board&, int) = manhattan_distance) {

    u256 goal_packed = pack(make_goal(n), n * n);
    u256 start_packed = pack(start, n * n);
    int size = n * n;
    int bits = bit_length((uint64_t)(size - 1));

    if (start_packed == goal_packed) {
        out_path = {start};
        return true;
    }

    std::unordered_map<u256, u256, u256_hash> came_from;
    std::unordered_map<u256, int, u256_hash> g_score;
    std::unordered_set<u256, u256_hash> closed;
    g_score[start_packed] = 0;
    int h0 = heuristic(start, n);
    std::priority_queue<HeapItem, std::vector<HeapItem>, HeapCmp> heap;
    heap.push({h0, 0, start_packed});
    // vectors
    static const int DIRS[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    while (!heap.empty()) {
        HeapItem top = heap.top();
        heap.pop();
        int g = top.g;
        u256 packed = top.p;

        if (packed == goal_packed) {
            // Walk came_from (all packed ints) and unpack at reconstruction time.
            Path path;
            u256 cur = packed;

            while (came_from.find(cur) != came_from.end()) {
                path.push_back(unpack(cur, n));
                cur = came_from[cur];
            }

            path.push_back(unpack(cur, n));
            std::reverse(path.begin(), path.end());
            out_path = std::move(path);
            return true;
        }

        if (closed.count(packed)) continue;

        closed.insert(packed);
        int blank = packed_blank(packed, size, bits);
        int row = blank / n, col = blank % n;

        for (auto& d : DIRS) {

            int nr = row + d[0], nc = col + d[1];
            if (!(0 <= nr && nr < n && 0 <= nc && nc < n)) continue;
            u256 nb = pack_swap(packed, blank, nr * n + nc, bits);
            if (closed.count(nb)) continue;
            int tg = g + 1;
            auto it = g_score.find(nb);
            int best = (it == g_score.end()) ? 1000000000 : it->second;

            if (tg < best) {
                g_score[nb] = tg;
                came_from[nb] = packed;
                int h = heuristic(unpack(nb, n), n);
                heap.push({tg + h, tg, nb});
            }
        }
    }
    return false;
}

// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Console display
// ---------------------------------------------------------------------------

/*
print_board(state, n)
-----------------------------------------------------------------------------
Formats a single board state as a human-readable n*n grid. The blank tile
is displayed as '_' (one character) so it visually stands out from numbered
tiles and the grid columns stay aligned.
-----------------------------------------------------------------------------
*/
inline std::string rjust(const std::string& s, int w) {
    if ((int)s.size() >= w) return s;
    return std::string(w - s.size(), ' ') + s;
}
inline std::string center(const std::string& s, int w) {
    if ((int)s.size() >= w) return s;
    int pad = w - (int)s.size();
    int left = pad / 2;
    return std::string(left, ' ') + s + std::string(pad - left, ' ');
}

inline void print_board(const Board& state, int n) {
    int w = (int)std::to_string(n * n - 1).size();
    for (int r = 0; r < n; ++r) {
        std::string row_str;
        for (int c = 0; c < n; ++c) {
            if (c) row_str += "  ";
            int v = state[r * n + c];
            row_str += (v == 0) ? center("_", w) : rjust(std::to_string(v), w);
        }
        printf("%s\n", row_str.c_str());
    }
}

inline void print_solution(const Path* path, int n) {
    if (path == nullptr || path->empty()) {
        printf("No solution found.\n");
        return;
    }
    for (size_t step = 0; step < path->size(); ++step) {
        const char* tag = step == 0 ? " (initial)"
                          : (step == path->size() - 1 ? " (goal)" : "");
        printf("\nStep %zu%s:\n", step, tag);
        print_board((*path)[step], n);
    }
    printf("\nSolved in %zu move(s).\n", path->size() - 1);
}
