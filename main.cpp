// Entry

#include "npuzzle.hpp"
#include "visuals.hpp"
#include <cstdio>

void solve(const Board& initial, int n) {

    if (!is_solvable(initial, n)) {
        printf("This configuration is not solvable.\n");
        return;
    }

    printf("Solving...\n");
    Path path;
    bool ok = astar(initial, n, path);
    print_solution(ok ? &path : nullptr, n);

    if (ok && !path.empty()) {
        visuals::visualize(path, n);
    }
}


int main() {

    int n = 3;
    bool easy = true;
    Board state;

    if (n >= 4 && easy) {
        state = easy_state(n)
    } else {
        state = random_state(n);
    }

    solve(state, n);
    return 0;
}
