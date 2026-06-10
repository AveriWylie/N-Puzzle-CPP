# (n²-1)-Puzzle A* Visualizer — C++

A C++ port of the Python A* sliding-tile solver, mapped 1:1: same board
representation, same search, same solvability test, same animation. The
visualization uses raylib in place of pygame.

## Why C++

The packed board needs `n²·bits` bits where `bits = (n²-1).bit_length()`:
64 at n=4, **125 at n=5, 216 at n=6**. Python's arbitrary-width int absorbs
that for free; here the packed state lives in a fixed 256-bit value
(`u256`, four `uint64_t` words) copied by value and hashed flat, with no
per-node allocation in the A* loop. That removes the GC/boxing pressure a
managed runtime would add in exactly the place A* is memory-bound, which is
what lets larger boards stay viewable.

## Layout

- `src/u256.hpp` — fixed 256-bit packed integer (stands in for Python's int)
- `src/npuzzle.hpp` — board representation, A*, heuristics, solvability, console display
- `src/visuals.hpp` / `src/visuals.cpp` — raylib rendering and animation (port of `visuals.py`)
- `src/main.cpp` — entry point (port of the `__main__` block + `solve`)
- `test.cpp` — headless correctness checks (no window)

## Build

```bash
cmake -B build
cmake --build build -j
```

If raylib is not installed system-wide, CMake fetches and builds it
automatically (needs the usual OpenGL/X11 dev packages on Linux, or nothing
extra on macOS/Windows toolchains).

## Run

```bash
./build/npuzzle        # opens the animated window
./build/npuzzle_test   # runs the headless correctness checks
```

## Usage

Set `n` and the `easy` flag in `main`. For `n >= 4`, `easy = true` uses the
bounded scramble; `easy = false` uses a full random state. For `n < 4` it
always uses a random state.

### Controls

- `SPACE` / `ENTER` skip the intro pause and start the animation
- `ESC` or closing the window quits

## What it shows

- **Deciding solvability is linear time**: a single inversion-count parity check.
- **Finding the optimal solution is NP-hard** for the generalized
  (n²-1)-puzzle. A* with Manhattan distance is optimal, but its time and
  memory grow exponentially with the optimal solution depth, which you can
  watch as `n` scales from 3×3 (instant) toward 5×5/6×6 random states
  (exponential blowup). Easy mode caps the scramble depth so large boards
  stay tractable.
