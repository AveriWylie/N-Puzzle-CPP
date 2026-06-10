#pragma once
#include "npuzzle.hpp"

// Global Variables
constexpr int GAP = 8;
constexpr int INFO_H = 72;
constexpr int FPS = 60;
constexpr int ANIM_FRAMES = 20;  // frames spent sliding the tile  (~333 ms at 60 fps)
constexpr int HOLD_FRAMES = 6;   // frames the settled state is shown before the next move

namespace visuals {

/*
Animate the solution path in a raylib window.

Each move is triggered as we "pop" the next state from the solution list.
The animation runs at FPS fps with ANIM_FRAMES frames of sliding
motion followed by HOLD_FRAMES frames of the settled board.

Controls:
    SPACE / ENTER  -- skip the intro pause and start immediately
    ESC / close    -- quit
*/
void visualize(const Path& path, int n);

}  // namespace visuals
