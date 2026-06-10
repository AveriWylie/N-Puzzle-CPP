#include "visuals.hpp"
#include "raylib.h"
#include <string>
#include <cmath>
#include <algorithm>

namespace visuals
{
    static int _tile_size(int n) {
        // Compute a tile pixel size that keeps the board around 460 px wide.
        int board_target = 460;
        return (board_target - (n + 1) * GAP) / n;
    }

    /*
    Map tile value v (1..total) to an RGB color via HSV.
    The hue starts at blue-violet and shifts through the full spectrum.
    */
    static Color tile_color(int v, int total) {
        double hue = std::fmod(0.62 + (double)(v - 1) / std::max(total, 1) * 0.82, 1.0);
        double s = 0.68, val = 0.90;
        // colorsys.hsv_to_rgb
        double r, g, b;

        if (s == 0.0) {
            r = g = b = val;

        } else {
            int i = (int)(hue * 6.0);
            double f = hue * 6.0 - i;
            double p = val * (1.0 - s);
            double q = val * (1.0 - s * f);
            double t = val * (1.0 - s * (1.0 - f));
            i %= 6;

            switch (i) {
            case 0: r = val; g = t;   b = p;   break;
            case 1: r = q;   g = val; b = p;   break;
            case 2: r = p;   g = val; b = t;   break;
            case 3: r = p;   g = q;   b = val; break;
            case 4: r = t;   g = p;   b = val; break;
            default: r = val; g = p;  b = q;   break;
            }

        }
        return Color{(unsigned char)(r * 255), (unsigned char)(g * 255), (unsigned char)(b * 255), 255};
    }

    // raylib roundness for a target corner radius on a w x h rectangle
    static float roundness_for(float radius, float w, float h) {
        float halfmin = std::min(w, h) * 0.5f;
        if (halfmin <= 0) return 0.0f;
        float r = radius / halfmin;

        return r > 1.0f ? 1.0f : r;
    }

    /*
    Render a single tile at pixel position (x, y) with:
      - a dark drop-shadow offset by (3, 5)
      - a filled rounded rectangle in the tile's HSV color
      - a lighter highlight strip across the top edge
      - a centered white number label
    */
    static void draw_tile(int v, int x, int y, int tile, Font font, int font_sz, int total) {

        Color color = tile_color(v, total);
        // Drop shadow
        DrawRectangleRounded(Rectangle{(float)(x + 3), (float)(y + 5), (float)tile, (float)tile},
                                roundness_for(12, tile, tile), 8, Color{6, 6, 12, 255});
        // Fill
        DrawRectangleRounded(Rectangle{(float)x, (float)y, (float)tile, (float)tile},
                                roundness_for(12, tile, tile), 8, color);
        // Top highlight strip
        Color hi{(unsigned char)std::min(255, color.r + 50),c(unsigned char)std::min(255, color.g + 50),
                                (unsigned char)std::min(255, color.b + 50), 255};

        DrawRectangleRounded(Rectangle{(float)(x + 8), (float)(y + 6), (float)(tile - 16), 5.0f},
                                roundness_for(3, tile - 16, 5), 4, hi);
        // Number
        std::string s = std::to_string(v);
        Vector2 dim = MeasureTextEx(font, s.c_str(), (float)font_sz, 1.0f);

        DrawTextEx(font, s.c_str(), Vector2{x + (tile - dim.x) / 2.0f, y + (tile - dim.y) / 2.0f},
                                (float)font_sz, 1.0f, Color{248, 248, 255, 255});
    }

    static void find_move(const Board& a, const Board& b, int n, int& mv, int& from_flat, int& to_flat) {

        int blank_a = (int)(std::find(a.begin(), a.begin() + n * n, 0) - a.begin());
        int blank_b = (int)(std::find(b.begin(), b.begin() + n * n, 0) - b.begin());
        mv = a[blank_b];
        from_flat = blank_b;
        to_flat = blank_a;
    }

    struct Moving {
        int v, from_i, to_i;
        float progress;
        bool active;
    };

    /*
    Render one display frame.

    Draws every tile at its position in `state`, except the tile identified
    by `moving`, which is instead rendered at its smoothstep-interpolated
    position between from_flat and to_flat.

    Parameters:
        moving  -- (tile_value, from_flat, to_flat, progress in [0,1]) or inactive
        solved  -- if True the info bar shows the "Solved!" message
    */
    static void draw_frame(const Board& state, int n, int tile, Font font, int font_sz, Font info_font, int info_sz, int step, int total_steps, Moving moving, bool solved) {

        int board_px = n * tile + (n + 1) * GAP;
        int total = n * n - 1;
        ClearBackground(Color{14, 14, 22, 255});

        for (int i = 0; i < n * n; ++i) {
            int r = i / n, c = i % n;
            int x = GAP + c * (tile + GAP);
            int y = GAP + r * (tile + GAP);

            DrawRectangleRounded(Rectangle{(float)x, (float)y, (float)tile, (float)tile},
                                 roundness_for(12, tile, tile), 8, Color{28, 28, 44, 255});

        }

        // -- Static tiles --
        int skip_v = moving.active ? moving.v : -1;

        for (int idx = 0; idx < n * n; ++idx) {
            int v = state[idx];
            if (v == 0 || v == skip_v) continue;
            int r = idx / n, c = idx % n;
            draw_tile(v, GAP + c * (tile + GAP), GAP + r * (tile + GAP), tile, font, font_sz, total);
        }

        // -- Animated tile --
        if (moving.active) {
            int from_i = moving.from_i, to_i = moving.to_i;
            int fr = from_i / n, fc = from_i % n;
            int tr = to_i / n, tc = to_i % n;
            int fx = GAP + fc * (tile + GAP);
            int fy = GAP + fr * (tile + GAP);
            int tx = GAP + tc * (tile + GAP);
            int ty = GAP + tr * (tile + GAP);
            // Smoothstep: ease in and out
            float prog = moving.progress;
            float p = prog * prog * (3.0f - 2.0f * prog);
            int cx = (int)(fx + (tx - fx) * p);
            int cy = (int)(fy + (ty - fy) * p);
            draw_tile(moving.v, cx, cy, tile, font, font_sz, total);
        }

        // -- Info bar --
        int bar_y = board_px;
        DrawRectangle(0, bar_y, board_px, INFO_H, Color{20, 20, 32, 255});

        std::string msg;
        Color color;

        if (solved) {
            msg = "Solved in " + std::to_string(total_steps) + " move" + (total_steps != 1 ? "s" : "") + "!";
            color = Color{85, 255, 145, 255};

        } else {
            msg = "Move  " + std::to_string(step) + "  /  " + std::to_string(total_steps);
            color = Color{160, 160, 210, 255};
        }

        Vector2 dim = MeasureTextEx(info_font, msg.c_str(), (float)info_sz, 1.0f);
        DrawTextEx(info_font, msg.c_str(), Vector2{board_px / 2.0f - dim.x / 2.0f, bar_y + INFO_H / 2.0f - dim.y / 2.0f}, (float)info_sz, 1.0f, color);
    }

    void visualize(const Path& path, int n) {

        int tile = _tile_size(n);
        int board_px = n * tile + (n + 1) * GAP;
        int total = (int)path.size() - 1;

        std::string title = std::to_string(n * n - 1) + "-Puzzle  -  A*";
        InitWindow(board_px, board_px + INFO_H, title.c_str());
        SetTargetFPS(FPS);

        int font_sz = std::max(24, tile / 2 + 2);
        int info_sz = 24;
        Font font = GetFontDefault();
        Font info_font = GetFontDefault();

        enum Phase {
            INTRO, ANIMATING, HOLDING, SOLVED
        };

        Phase phase = (total == 0) ? SOLVED : INTRO;
        int step = 0;
        int frame = 0;

        while (!WindowShouldClose()) {
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
                if (phase == INTRO) {
                    phase = ANIMATING;
                    frame = 0;
                }
            }

            if (phase == INTRO) {
                // ~1 second intro
                if (frame >= FPS) {
                    phase = ANIMATING;
                    frame = 0;
                }

            } else if (phase == ANIMATING) {
                if (frame >= ANIM_FRAMES) {
                    phase = HOLDING;
                    frame = 0;
                }

            } else if (phase == HOLDING) {
                if (frame >= HOLD_FRAMES) {
                    step += 1;
                    if (step >= total) phase = SOLVED;
                    else phase = ANIMATING;
                    frame = 0;
                }

            }

            BeginDrawing();
            Moving none{0, 0, 0, 0.0f, false};

            if (phase == INTRO || phase == SOLVED) {
                draw_frame(path[step], n, tile, font, font_sz, info_font, info_sz, step, total, none, phase == SOLVED);

            } else if (phase == ANIMATING) {
                const Board& state_a = path[step];
                const Board& state_b = path[step + 1];
                int mv, from_i, to_i;
                find_move(state_a, state_b, n, mv, from_i, to_i);

                float progress = (float)frame / ANIM_FRAMES;
                Moving moving{mv, from_i, to_i, progress, true};
                draw_frame(state_a, n, tile, font, font_sz, info_font, info_sz, step, total, moving, false);

                // holding
            } else {
                draw_frame(path[step + 1], n, tile, font, font_sz, info_font, info_sz, step + 1, total, none, false);
            }

            EndDrawing();
            frame += 1;
        }
        CloseWindow();
    }

} // namespace visuals