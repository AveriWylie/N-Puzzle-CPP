#include "npuzzle.hpp"
#include <iostream>
#include <map>

static int fails = 0;
#define CHECK(cond, msg) do { if(!(cond)){ std::cout << "FAIL: " << msg << "\n"; ++fails; } } while(0)

// second admissible heuristic, used to cross-check optimal length for n>=4
int misplaced_tiles(const Board& state, int n) {
    int c = 0;
    for (int i = 0; i < n*n; ++i) {
        int v = state[i];
        if (v && v != i + 1) ++c;
    }
    return c;
}

// independent BFS optimal-length oracle (small n only)
int bfs_optimal(const Board& start, int n) {
    if (!is_solvable(start, n)) return -1;
    Board goal = make_goal(n);
    if (start == goal) return 0;
    std::unordered_set<u256, u256_hash> seen;
    std::queue<std::pair<Board,int>> q;
    q.push({start, 0});
    seen.insert(pack(start, n*n));
    while (!q.empty()) {
        auto [s, d] = q.front(); q.pop();
        for (auto& nb : get_neighbors(s, n)) {
            if (nb == goal) return d + 1;
            u256 key = pack(nb, n*n);
            if (seen.insert(key).second) q.push({nb, d + 1});
        }
    }
    return -1;
}

// verify a path: starts at start, ends at goal, each step a single legal swap
bool valid_path(const Path& p, const Board& start, int n) {
    if (p.empty()) return false;
    if (!(p.front() == start)) return false;
    if (!(p.back() == make_goal(n))) return false;
    for (size_t i = 0; i + 1 < p.size(); ++i) {
        auto nbrs = get_neighbors(p[i], n);
        bool ok = false;
        for (auto& nb : nbrs) if (nb == p[i+1]) { ok = true; break; }
        if (!ok) return false;
    }
    return true;
}

int main() {
    // pack/unpack/pack_swap/packed_blank roundtrips, n = 3..6
    for (int n = 3; n <= 6; ++n) {
        Board s = random_state(n);
        u256 packed = pack(s, n*n);
        Board back = unpack(packed, n);
        for (int i = 0; i < n*n; ++i) CHECK(s[i] == back[i], "unpack roundtrip n=" << n);

        int bits = bit_length((uint64_t)(n*n - 1));
        // packed_blank matches the array blank index
        int bi = (int)(std::find(s.begin(), s.begin()+n*n, 0) - s.begin());
        CHECK(packed_blank(packed, n*n, bits) == bi, "packed_blank n=" << n);

        // pack_swap matches swap_tiles + pack
        int j = (bi + 1) % (n*n);
        u256 sw = pack_swap(packed, bi, j, bits);
        Board sw2 = swap_tiles(s, bi, j);
        CHECK(sw == pack(sw2, n*n), "pack_swap n=" << n);
    }

    // is_solvable agrees with BFS reachability on n=3 (exhaustive-ish sample)
    {
        int n = 3, agree = 0, total = 0;
        for (int t = 0; t < 200; ++t) {
            Board s = random_state(n);
            bool solv = is_solvable(s, n);
            bool reach = (bfs_optimal(s, n) >= 0);
            if (solv == reach) ++agree;
            ++total;
        }
        CHECK(agree == total, "is_solvable vs BFS reachability (" << agree << "/" << total << ")");
    }

    // A* finds optimal solutions matching BFS, and emits valid paths (n=3)
    {
        int n = 3, checked = 0;
        for (int t = 0; t < 60; ++t) {
            Board s = random_state(n);
            if (!is_solvable(s, n)) continue;
            int opt = bfs_optimal(s, n);
            Path p;
            bool ok = astar(s, n, p);
            CHECK(ok, "astar returns a path for solvable n=3");
            CHECK((int)p.size() - 1 == opt, "astar optimal len " << (p.size()-1) << " vs bfs " << opt);
            CHECK(valid_path(p, s, n), "astar path valid n=3");
            ++checked;
        }
        std::cout << "checked " << checked << " solvable 3x3 instances\n";
    }

    // A* optimal on 4x4 easy scrambles: manhattan vs misplaced-tiles must agree
    {
        int n = 4, checked = 0;
        for (int t = 0; t < 8; ++t) {
            Board s = easy_state(n);
            Path pm, pt;
            bool okm = astar(s, n, pm, manhattan_distance);
            bool okt = astar(s, n, pt, misplaced_tiles);
            CHECK(okm && okt, "astar path for easy 4x4");
            CHECK(pm.size() == pt.size(), "astar 4x4 optimal agree mh=" << (pm.size()-1) << " mt=" << (pt.size()-1));
            CHECK(valid_path(pm, s, n), "astar 4x4 path valid");
            ++checked;
        }
        std::cout << "checked " << checked << " easy 4x4 instances (two-heuristic agreement)\n";
    }

    // 5x5 and 6x6 easy: representation works past 64/128 bits, path valid
    for (int n = 5; n <= 6; ++n) {
        Board s = easy_state(n);
        Path p;
        bool ok = astar(s, n, p);
        CHECK(ok, "astar easy n=" << n);
        CHECK(valid_path(p, s, n), "astar easy path valid n=" << n);
        std::cout << "n=" << n << " easy solved in " << (p.size()-1) << " moves\n";
    }

    if (fails == 0) std::cout << "\nALL TESTS PASSED\n";
    else std::cout << "\n" << fails << " CHECK(S) FAILED\n";
    return fails ? 1 : 0;
}
