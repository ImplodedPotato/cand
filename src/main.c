#include <stdlib.h>
#include <stdio.h>

#include "raylib.h"
#include "raymath.h"

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const float TPS = 20.f;

typedef struct Grid {
    unsigned int *data;
    int bpp;
    int width;
    int len;
} Grid;


Grid new_grid(const int bpp) {
    const int width = WINDOW_WIDTH / bpp;
    const int height = WINDOW_HEIGHT / bpp;
    const int len = width * height;
    const Grid grid = {
        .data = calloc(len, sizeof(int)),
        .bpp = bpp,
        .width = width,
        .len = len
    };
    return grid;
}

void update_hovered_tile(const Grid *grid);
void draw_grid(const Grid *grid);
void place_sand(const Grid *grid);
void update_gravity(const Grid *grid);
void reset_data(Grid *grid);
int inverse(int x, int min, int max);
float inversef(float x, float min, float max);

int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "cand");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);

    Grid grid = new_grid(40);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_R)) reset_data(&grid);

        update_hovered_tile(&grid);
        place_sand(&grid);
        update_gravity(&grid);

        BeginDrawing();
        ClearBackground(BLACK);

        draw_grid(&grid);

        DrawFPS(10, 10);
        EndDrawing();
    }

    free(grid.data);
    CloseWindow();
    return 0;
}

int get_hovered_index(const Grid *grid) {
    Vector2 pos = GetMousePosition();
    pos = (Vector2){
        .x = inversef(pos.x, 0, WINDOW_WIDTH),
        .y = inversef(pos.y, 0, WINDOW_HEIGHT),
    };
    if (pos.x <= 0 || pos.y <= 0) return 0;
    if (pos.x >= WINDOW_WIDTH || pos.y >= WINDOW_HEIGHT) return 0;

    const Vector2 xy = { floorf(pos.x / grid->bpp), floorf(pos.y/ grid->bpp) };
    return xy.x + (xy.y * grid->width);
}

#define HOVERED    0b10000000000000000000000000000000
#define SAND_WHITE 0b01000000000000000000000000000000

void update_hovered_tile(const Grid *grid) {
    static int prev = 0;

    const int index = get_hovered_index(grid);

    if (prev == index) return;
    grid->data[index] |= HOVERED;
    grid->data[prev]  &= ~(1 << 31); // set most significant bit (HOVERED) to 0
    prev = index;
}

// 1..=12 2=>11 4=>9 (x-13) 10=>3 abs(x-13)
// 0..=12 0=>12 12=>0
inline int inverse(const int x, const int min, const int max) {
    return max-(x-min);
}
inline float inversef(const float x, const float min, const float max) {
    return max-(x-min);
}

void draw_grid(const Grid *grid) {
    for (int i = 0; i < grid->len; ++i) {
        const int x = inverse(i % grid->width, 0, grid->width - 1)              * grid->bpp;
        const int y = inverse(i / grid->width, 0, grid->len / grid->width - 1)  * grid->bpp;

        if (grid->data[i] & ~(1 << 31)) {
            Color c = PURPLE;
            if (grid->data[i] & SAND_WHITE) c = RAYWHITE;
            DrawRectangle(x, y, grid->bpp, grid->bpp, c);
        }

        Color c = GRAY;
        if (grid->data[i] & HOVERED) c = ColorBrightness(c, 0.8f);
        DrawRectangleLines(x, y, grid->bpp, grid->bpp, c);
    }
}

void place_sand(const Grid *grid) {
    if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) return;

    const int index = get_hovered_index(grid);
    grid->data[index] |= SAND_WHITE;
}

void update_gravity(const Grid *grid) {
    static float c = 0;
    c += GetFrameTime();

    if (c <= 1.f / TPS) {
        return;
    }
    printf("ticked\n");
    c = 0;

    for (int i = 0; i < grid->len; ++i) {
        const int unhover = grid->data[i] & ~(1 << 31);
        if (!unhover) continue; // no sand

        const int below = i - grid->width;
        if (below < 0) continue; // on the floor
        if (grid->data[below] & ~(1 << 31)) {
            const unsigned int before = grid->data[below - 1] & ~(1 << 31);
            const unsigned int after  = grid->data[below + 1] & ~(1 << 31);
            if (before && after) continue;
            if (!before) {
                if (!after) {
                    const int r = (rand() % 2) ? 1 : -1;
                    grid->data[below + r] = unhover;
                } else {
                    grid->data[below - 1] = unhover;
                }
            } else {
                grid->data[below + 1] = unhover;
            }
        } else {
            grid->data[below] = unhover;
        }
        grid->data[i] &= HOVERED;
    }
}

inline void reset_data(Grid *grid) {
    free(grid->data);
    grid->data = calloc(grid->len, sizeof(int));
}