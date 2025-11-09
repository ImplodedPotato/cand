#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "raymath.h"
#include "types.h"

const i32 GRID_WIDTH  = 800;
const i32 GRID_HEIGHT = 600;
const i32 PPB = 10;
const f32 TPS = 60.f;
const u32 MAX_THREADS = 6;

typedef struct Grid {
    u32 *data;
    i32 pbb;
    f32 buff_pbb;
    u32 width;
    u32 len;
    u32 tps;
    u32 num;
    struct {
        u32 num;
        bool on;
        bool draw;
    } dbg;
    u32 selected_type;
    struct {
        u32 radius;
        f32 buff;
    } brush;
    u32 cores;
    Vector2 pos;
    bool line;
    bool tap;
    bool man_step;
    bool i_on_hov;
} Grid;


// TODO:
/*
typedef struct Px {
    u32 properties 0b10000000000000000000000000000000
    u8  direction  0b10000000  | NW N NE E SE S SW W |
    u16 pow        0b1000000000000000  number of tick the px should move in the provided direction
    *u8 of padding inserted by compiler, we can put extra data here if need be*
} Px;
use the pow and direction to determine where to push the px, or transfer pow to other pxs if blocks
*/

// Properties
#define HOVERED 0b10000000000000000000000000000000
#define UPDATED 0b01000000000000000000000000000000

#define LIQUID  0b00100000000000000000000000000000 // flows
#define SOLID   0b00010000000000000000000000000000 // stacks
#define GAS     0b00001000000000000000000000000000 // tbd
#define FLOAT   0b00000100000000000000000000000000 // float upon liquid
#define STILL   0b00000010000000000000000000000000 // Do not move the block

// Types
#define SOLID_WHITE  (0b00000000000000000000000000000001 | SOLID)
#define SOLID_RED    (0b00000000000000000000000000000010 | SOLID)
#define LIQUID_BLUE  (0b00000000000000000000000000000100 | LIQUID)
#define STILL_GREY   (0b00000000000000000000000000001000 | STILL)


Grid new_grid(const i32 ppb) {
    const i32 width = GRID_WIDTH / ppb;
    const i32 height = GRID_HEIGHT / ppb;
    const i32 len = width * height;

    const Grid grid = {
        .data = calloc(len, sizeof(i32)),
        .pbb = ppb,
        .buff_pbb = ppb,
        .width = width,
        .len = len,
        .tps = TPS,
        .selected_type = SOLID_WHITE,
        .dbg = {
            .on = false,
            .draw = true
        },
        .cores = MAX_THREADS,
    };
    return grid;
}

void free_grid(const Grid *grid) {
    free(grid->data);
}

void update_hovered_tile(Grid *grid);
void draw_grid(const Grid *grid);
void place_sand(Grid *grid);
void update_gravity(const Grid *grid);
void reset_data(Grid *grid);
i32 inverse(i32 x, i32 min, i32 max);
f32 inversef(f32 x, f32 min, f32 max);
void update_tps(Grid *grid);
void draw_extra_data(Grid *grid);
void draw_side_panel(Grid *grid);
void flop(u32 *lhs, u32 *rhs);
void print_bin(u32 num);
void print_biln(u32 num);
void update_real_num_dbg(Grid * grid);
bool check_for_update(Grid *grid);
void *draw_grid_slice(void *d);

i32 main(void) {
    // SetConfigFlags(FLAG_WINDOW_RESIZABLE); // TODO: support resizing
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(GRID_WIDTH + 364, GRID_HEIGHT + 16, "cand");
    // SetTargetFPS(60);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);

    Grid grid = new_grid(PPB);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_R)) reset_data(&grid);
        if (IsKeyPressed(KEY_G)) grid.line = !grid.line;

        update_tps(&grid);
        update_hovered_tile(&grid);

        place_sand(&grid);
        if (check_for_update(&grid)) {
            update_gravity(&grid);
            if (grid.dbg.on) {
                update_real_num_dbg(&grid);
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        if (grid.dbg.draw) {
            draw_grid(&grid);
        } else {
            for (u32 i = 0; i<grid.len; ++i) grid.data[i] &= ~UPDATED;
        }
        draw_side_panel(&grid);
        draw_extra_data(&grid);

        EndDrawing();
    }
    free_grid(&grid);
    CloseWindow();
    return 0;
}

bool check_for_update(Grid *grid) {
    static f32 c = 0;

    if (!grid->man_step) {
        c += GetFrameTime();

        // TODO: only account for tps 0 <= FPS
        //   change update gravity multiple times a tick, according to FPS & TPS
        if (c <= 1.f / (f32)grid->tps) {
            return false;
        }
        c = 0;
    } else {
        if (grid->tap) {
            if (!IsKeyPressed(KEY_N)) return false;
        } else {
            if (!IsKeyDown(KEY_N)) return false;
        }
    }
    return true;
}

void draw_extra_data(Grid *grid) {
    const f32 PADDING = 10;
    const f32 CHAR_WIDTH = 12.f;
    const f32 x = (f32)GRID_WIDTH - PADDING;
    f32 y = 0 + PADDING;
    f32 w = 60.f;
    const f32 h = 40.f;

    const char *text = TextFormat("TPS: %d", grid->tps);
    if (grid->man_step) { text = "TPS: MAN"; }
    w = (f32)TextLength(text) * CHAR_WIDTH;
    GuiDrawText(text, (Rectangle){ x - (f32)TextLength(text) * CHAR_WIDTH,  y, w, h },
        TEXT_ALIGN_RIGHT, WHITE);
    y += (h + PADDING) / 2;

    const char *num_text = TextFormat("Num: %d", grid->num);
    w = (f32)TextLength(num_text) * CHAR_WIDTH;
    GuiDrawText(num_text, (Rectangle){ x - (f32)TextLength(num_text) * CHAR_WIDTH,  y, w, h },
        TEXT_ALIGN_RIGHT, WHITE);
    y += (h + PADDING) / 2;

    if (grid->dbg.on) {
        const char *real_num_dbg_text = TextFormat("Real Num: %d", grid->dbg.num);
        w = (f32)TextLength(real_num_dbg_text) * CHAR_WIDTH;
        GuiDrawText(real_num_dbg_text, (Rectangle){ x - (f32)TextLength(real_num_dbg_text) * CHAR_WIDTH,  y, w, h },
            TEXT_ALIGN_RIGHT, WHITE);
    }

    if (grid->dbg.on) {
        DrawFPS(10, 10);
    }
}

void draw_side_panel(Grid *grid) {
    const f32 PADDING = 10.f;

    const Vector2 pos = GetMousePosition();

    const f32 x = GRID_WIDTH + PADDING;
    f32 y = 0 + PADDING;
    const f32 w = (f32)(GetScreenWidth() - GRID_WIDTH) - PADDING * 2;
    const f32 h = 40.f;

    GuiToggle((Rectangle){ x,  y, w, h },
        "Toggle GridLines", &grid->line);
    y += h + PADDING;

    if (GuiButton((Rectangle){ x,  y, w, h }, "Reset")) {
        reset_data(grid);
    }
    y += h + PADDING;

    const char *ppb_text = TextFormat("Pixels Per Bit: %.0f", grid->buff_pbb);
    GuiDrawText(ppb_text, (Rectangle){ x,  y, w, h }, TEXT_ALIGN_LEFT, WHITE);

    if ((i32)roundf(grid->buff_pbb) != grid->pbb) {
        const char *text = "Restart to\napply affects";
        GuiDrawText(text, (Rectangle){ x,  y + 5, w, h }, TEXT_ALIGN_RIGHT, WHITE);
    }
    y += h + PADDING;

    GuiSlider((Rectangle){ x,  y, w, h }, 0, 0,
        &grid->buff_pbb, 1, 100);
    y += h + PADDING;

    const Color colors[] = { WHITE,        RED,       BLUE,        DARKGRAY }; // TODO: covert to i32 map?
    const i32 ccs[]      = { SOLID_WHITE,  SOLID_RED, LIQUID_BLUE, STILL_GREY };
    for (u32 i = 0; i < sizeof(colors)/sizeof(Color); ++i) {
        const Rectangle bb = (Rectangle){ x + (float)i*(h + PADDING),  y, h, h };
        GuiDrawRectangle(bb,4, ColorBrightness(colors[i], -0.2f), colors[i]);
        if (CheckCollisionPointRec(pos, bb) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            grid->selected_type = ccs[i];
    }
    y += h + PADDING;

    GuiToggle((Rectangle){ x,  y, w, h },
        "Toggle Tapping", &grid->tap);
    y += h + PADDING;

    GuiToggle((Rectangle){ x,  y, w, h },
        "Toggle Man Stepping", &grid->man_step);
    y += h + PADDING;

    GuiToggle((Rectangle){ x,  y, w, h },
        "Toggle Index On Hover", &grid->i_on_hov);
    y += h + PADDING;

    const char *brush_size_text = TextFormat("Brush Size: %.0fpx", grid->brush.buff);
    GuiDrawText(brush_size_text, (Rectangle){ x,  y, w, h }, TEXT_ALIGN_LEFT, WHITE);
    y += h + PADDING;
    if (GuiSlider((Rectangle){ x,  y, w, h }, 0, 0,
        &grid->brush.buff, 1, 100)) {
        grid->brush.radius = (u32)floorf(grid->brush.buff);
    }
    y += h + PADDING;

    GuiToggle((Rectangle){ x,  y, w, h },
        "Toggle dbg", &grid->dbg.on);
    y += h + PADDING;

    GuiToggle((Rectangle){ x,  y, w, h },
        "Toggle Drawing", &grid->dbg.draw);
}

i32 get_hovered_index(const Grid *grid) {
    Vector2 pos = GetMousePosition();
    const f32 height = (f32)grid->len / (f32)grid->width * grid->pbb;
    const f32 width  = grid->width * grid->pbb;
    pos = (Vector2){
        .x = inversef(pos.x, 0,  width),
        .y = inversef(pos.y, 0, height),
    };
    if (pos.x <= 0 || pos.y <= 0)          return -1;
    if (pos.x >= width || pos.y >= height) return -1;

    const Vector2 xy = { floorf(pos.x / grid->pbb), floorf(pos.y/ grid->pbb) };
    return xy.x + (xy.y * grid->width);
}

void update_hovered_tile(Grid *grid) {
    u32 num = 0;

    const i32 hovered = get_hovered_index(grid);
    const u32 r = grid->brush.radius - 1;

    const Vector2 xy = (Vector2){ hovered % grid->width, hovered / grid->width };

    for (u32 i = 0; i < grid->len; ++i) {
        // if (num = r*r) return;
        const Vector2 ixy = (Vector2){ i % grid->width, i / grid->width };
        if ((ixy.x < xy.x - r || ixy.y < xy.y - r) ||
            (ixy.x > xy.x + r || ixy.y > xy.y + r)) {
            grid->data[i] &= ~HOVERED;
            num++;
            continue;
        }

        grid->data[i] |= HOVERED;
    }
}

void update_tps(Grid * grid) {
    if (IsKeyDown(KEY_MINUS) && grid->tps > 0) grid->tps--;
    if (IsKeyDown(KEY_EQUAL)) grid->tps++;
}

// 1..=12 2=>11 4=>9 (x-13) 10=>3 abs(x-13)
// 0..=12 0=>12 12=>0
inline i32 inverse(const i32 x, const i32 min, const i32 max) {
    return max-(x-min);
}
inline f32 inversef(const f32 x, const f32 min, const f32 max) {
    return max-(x-min);
}

void draw_grid(const Grid *grid) {
    const u32 pbb = grid->pbb;
    for (u32 i = 0; i < grid->len; ++i) {
        const i32 x = inverse(i % grid->width, 0, (i32)grid->width - 1)                * pbb;
        const i32 y = inverse(i / grid->width, 0, (i32)(grid->len / grid->width - 1))  * pbb;
        const u32 unhovered = grid->data[i] & ~HOVERED & ~UPDATED;


        if (unhovered) {
            Color c = PURPLE;
            if (unhovered == SOLID_WHITE)  c = RAYWHITE;
            if (unhovered == SOLID_RED)    c = RED;
            if (unhovered == LIQUID_BLUE)  c = BLUE;
            if (unhovered == STILL_GREY)   c = DARKGRAY;
            if (pbb <= 1) {
                DrawPixel(x, y, c);
            } else {
                DrawRectanglePro((Rectangle){(f32)x, (f32)y, (f32)pbb, (f32)pbb},
                    Vector2Zero(), 0.f, c);
            }
        }

        // TODO: find a more efficient way of drawing the grid
        if (grid->line && pbb > 3) {
            Color c = GRAY;
            if (grid->data[i] & HOVERED) c = ColorBrightness(c, 0.8f);
            DrawRectangleLines(x, y, pbb, pbb, c);
        } else {
            if (grid->data[i] & HOVERED) {
                DrawRectangleLines(x, y, pbb, pbb, GRAY);
            }
        }

        if (grid->i_on_hov && grid->data[i] & HOVERED) {
            const char *text = TextFormat("%d", i);
            DrawText(text, x, y, 20, LIGHTGRAY);
        }

        grid->data[i] &= ~UPDATED;
    }

    DrawRectangleLines(grid->pos.x, grid->pos.y, grid->width * pbb, grid->len / grid->width * pbb, GRAY);
}

void place_sand(Grid *grid) {
    if (grid->tap) {
        if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;
    } else {
        if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) return;
    }

    // TODO: would it worth it to make a get_hovered()?
    //   This would use get_hovered_index() and use math with grid->brush.size
    //   to return an array of the indexes of all hovered pxs.
    for (u32 i = 0, c = 0; i < grid->len; ++i) {
        if (!(grid->data[i] & HOVERED) || grid->data[i] & ~HOVERED) continue;

        grid->data[i] |= grid->selected_type;
        grid->num++;
        c++;
    }
}

inline void flop(u32 *lhs, u32 *rhs) {
    const u32 tmp = *lhs;
    *lhs = *rhs;
    *rhs = tmp;
}

// for the given liquid block (index), displace it to the closest empty px
/*
    Key:
        0 => empty
        1 => sand
        W => liquid
        S => target liquid (start)
        F => displaces to  (finish)

    0 0 0 0 1 1 1 1 0 0 0

    0 0 0 0 0 1 1 1 0 0 0

    0 F 0 0 0 F 1 1 F 0 0
      |       |     |
    W W W W W W-S 1 W W W
      |         |   |
    W W 1 W W W S S-W W W
      |
    W W-S W W W W W W W W
*/
// treat the liquid like air, except you call this fn right before moving the block
//   as to move it into air, and not replace the liquid
i32 displace_liquid(const Grid *grid, const u32 index, const u32 max_iter) {
    u32 mov = index;
    i32 dir = grid->width;
    for (u32 i = 0; i <= max_iter; ++i) {
        if (i >= max_iter - 1) return -1;
        if (mov + dir < 0) mov = grid->width - 1;
        if (mov + dir > grid->len) mov = grid->len - grid->width + 1;

        const u32 unhover = grid->data[mov] & ~HOVERED; // 0b01000001

        if (!unhover) break;

        u32 before = 0;
        if (mov <= 0)
            before = grid->data[grid->width - 1] & ~HOVERED;
        else
            before = grid->data[mov - 1] & ~HOVERED;

        u32 after = 0;
        if (mov >= grid->len)
            after = grid->data[grid->len - grid->width + 1] & ~HOVERED;
        else
            after = grid->data[mov + 1] & ~HOVERED;

        u32 above = grid->data[mov] & ~HOVERED;
        if (mov / grid->width > grid->len / grid->width)
            above = grid->data[mov + grid->width] & ~HOVERED;

        // THIS WORKS
        if (grid->data[mov+dir] & ~HOVERED & SOLID) { // is blocked
            // change dir
            if (!(above & LIQUID) || !above) {
                if (!before) {
                    if (!after) {
                        const i32 r = (rand() % 2) ? 1 : -1;
                        dir = r; // Before & After
                    } else {
                        dir = 1; // Before
                    }
                } else {
                    dir = -1; // After
                }
            } else {
                dir = grid->width; // Above
            }
        }
        if (!(grid->data[mov+grid->width] & ~HOVERED))
            dir = grid->width;
        mov += dir;
    }
    grid->data[mov] = grid->data[index] & ~HOVERED;
    grid->data[mov] |= UPDATED;
    grid->data[index] &= HOVERED;
    return (i32)mov;
}

// careful of memory leak!
char *bin(const u32 num) {
    char *buff = calloc(32, sizeof(char));
    for (int i = sizeof(u32) * 8 - 1; i > 0; i--) {
        buff[31 - i] = *TextFormat("%d", (num >> i) & 1);
    }
    return buff;
}

void print_bin(const u32 num) {
    char *b = bin(num);
    printf("0b%s", b);
    free(b);
}

void print_biln(const u32 num) {
    char *b = bin(num);
    printf("0b%s\n", b);
    free(b);
}

void update_real_num_dbg(Grid * grid) {
    u32 num = 0;

    for (int i = 0; i < grid->len; ++i) {
        if (!(grid->data[i] & ~HOVERED)) continue; // no sand
        num++;
    }
    grid->dbg.num = num;
}

void update_gravity(const Grid *grid) {
    for (int i = 0; i < grid->len; ++i) {
        const u32 unhover = grid->data[i] & ~HOVERED;
        if (!unhover) continue; // no sand
        if (unhover & UPDATED) continue;

        if (unhover & STILL) continue;

        const i32 below_i = i - grid->width;

        if ((unhover & LIQUID) && (grid->data[i + grid->width] & SOLID)) {
            const i32 mov = displace_liquid(grid, i, 6);
            if (mov >= 0 ) {
                grid->data[i] = grid->data[i + grid->width] & ~HOVERED | UPDATED;
                grid->data[i + grid->width] &= HOVERED;
            } else {
                flop(&grid->data[i], &grid->data[i+grid->width]);
            }
            continue;
        }

        if (below_i < 0) continue; // on the floor
        if (grid->data[below_i] & ~HOVERED) {
            const u32 before = grid->data[below_i - 1] & ~HOVERED;
            const u32 after  = grid->data[below_i + 1] & ~HOVERED;

            // flowy liquids
            if (before && after) {
                if (!(unhover & LIQUID)) continue;
                i32 r = (rand() % 2) ? 1 : -1;
                if (grid->data[i + r] & ~HOVERED) r = -r;
                if (grid->data[i + r] & ~HOVERED) continue;
                grid->data[i + r] = unhover | UPDATED;
                grid->data[i] &= HOVERED;
                continue;
            }

            const i32 t;

            // sand & liquids
            if (!before) {
                if (!after) { // Before & After
                    const i32 r = (rand() % 2) ? 1 : -1;
                    const i32 target = below_i + r;
                    grid->data[target] = unhover | UPDATED;
                } else { // After
                    const i32 target = below_i - 1;
                    grid->data[target] = unhover | UPDATED;
                }
            } else { // Before
                const i32 target = below_i + 1;
                grid->data[target] = unhover | UPDATED;
            }
        } else {
            grid->data[below_i] = unhover | UPDATED;
        }

        grid->data[i] &= HOVERED;
    }
}

void reset_data(Grid *grid) {
    free(grid->data);
    const i32 pbb    = (i32)roundf(grid->buff_pbb);
    const i32 width  = GRID_WIDTH  / pbb;
    const i32 height = GRID_HEIGHT / pbb;
    const i32 len = width * height;
    grid->data  = calloc(len, sizeof(i32));
    grid->pbb   = pbb;
    grid->len   = len;
    grid->width = width;
    grid->num   = 0;
}
