#include "raylib.h"
#include "math.h"
#include <cstdint>
#include <unordered_map>
#include <map>
#include <vector>
#include <iostream>

const Color bg = {28, 30, 36, 255};
const Color lines = {42, 46, 54, 255};
const Color hl_fill = {70, 78, 96, 90};
const Color hl_edge = {110, 124, 150, 255};

enum class Dir: uint8_t {RIGHT, DOWN, LEFT, UP};
Dir dir_set[4] = {Dir::RIGHT, Dir::DOWN, Dir::LEFT, Dir::UP};

enum class Celltype: uint8_t {GENERATOR, MOVER, ROTATOR_CW, ROTATOR_CCW, PUSH, SLIDE, WALL, TRASH, ENEMY};
struct Cell {Celltype type; Dir dir = Dir::RIGHT; };

struct Celldata { KeyboardKey key; const char* filename; Texture2D texture; };
std::map<Celltype, Celldata> cell_data = {
    {Celltype::GENERATOR, {KEY_ONE, "generator.png", {0}}},
    {Celltype::MOVER, {KEY_TWO, "mover.png", {0}}},
    {Celltype::ROTATOR_CW, {KEY_THREE, "rotator_cw.png", {0}}},
    {Celltype::ROTATOR_CCW, {KEY_FOUR, "rotator_ccw.png", {0}}},
    {Celltype::PUSH, {KEY_FIVE, "push.png", {0}}},
    {Celltype::SLIDE, {KEY_SIX, "slide.png", {0}}},
    {Celltype::WALL, {KEY_SEVEN, "wall.png", {0}}},
    {Celltype::TRASH, {KEY_EIGHT, "trash.png", {0}}},
    {Celltype::ENEMY, {KEY_NINE, "enemy.png", {0}}}
};

struct Cord2d {
    int x, y;
    bool operator==(const Cord2d &o) const { return x == o.x && y == o.y; }
};

struct Cord2dHash {
    size_t operator()(const Cord2d &c) const {
        size_t h = (size_t)c.x * 73856093U ^ (size_t)c.y * 19349663U;
        return h;
    }
};

static std::unordered_map<Cord2d, Cell, Cord2dHash> grid;

Dir dir_selected = Dir::RIGHT;
Celltype celltype_selected = Celltype::MOVER;

Color get_cell_color(Cell c) {
    if (c.type == Celltype::MOVER) return ORANGE;
    else return WHITE;
}

Dir rotate_dir(Dir d, int times) {
    int idx;
    for (idx = 0; idx < 4; idx++) {
        if (dir_set[idx] == d) break;
    }
    int upd_idx = idx + times;
    if (upd_idx >= 0) return dir_set[upd_idx % 4];
    else return dir_set[(2 - upd_idx) % 4];
}
int dir_to_degrees(Dir d) {
    int idx;
    for (idx = 0; idx < 4; idx++) {
        if (dir_set[idx] == d) break;
    }
    return idx * 90;
}

Dir reverse(Dir dir) {
    if (dir == Dir::RIGHT) return Dir::LEFT;
    else if (dir == Dir::LEFT) return Dir::RIGHT;
    else if (dir == Dir::UP) return Dir::DOWN;
    else return Dir::UP;
}

Cord2d offset(Cord2d pos, Dir dir) {
    if (dir == Dir::RIGHT) return {pos.x+1, pos.y};
    else if (dir == Dir::LEFT) return {pos.x-1, pos.y};
    else if (dir == Dir::UP) return {pos.x, pos.y-1};
    return {pos.x, pos.y+1};
}

void push(Cord2d pos, Dir cur_dir, int force, Cell* replacement) {
    Cord2d pos_next = offset(pos, cur_dir);
    if (grid[pos].type == Celltype::WALL) return;

    while (grid.count(pos_next) != 0 && grid[pos_next].type != Celltype::TRASH && grid[pos_next].type != Celltype::ENEMY) {
        if (grid[pos_next].type == Celltype::MOVER) {
            if (grid[pos_next].dir == cur_dir) force++;
            else if (grid[pos_next].dir == reverse(cur_dir)) force--;
        }
        if (force < 0 || grid[pos_next].type == Celltype::WALL || grid[pos_next].type == Celltype::TRASH) return;
        pos_next = offset(pos_next, cur_dir);
    }

    if (grid[pos_next].type == Celltype::TRASH) {
        grid.erase(offset(pos_next, reverse(cur_dir)));
    } else if (grid[pos_next].type == Celltype::ENEMY) {
        grid.erase(pos_next);
        grid.erase(offset(pos_next, reverse(cur_dir)));
    }

    pos_next = offset(pos_next, reverse(cur_dir));

    

    while (!(pos_next == pos)) {
        if (grid.count(pos_next)) grid[offset(pos_next, cur_dir)] = grid[pos_next];
        pos_next = offset(pos_next, reverse(cur_dir));
    }
    if (grid.count(pos_next)) grid[offset(pos_next, cur_dir)] = grid[pos_next];
    grid.erase(pos);
    if (replacement) grid[pos] = *replacement;
    return;
}

void process_cell(Dir d, Cord2d pos) {
    if (grid.find(pos) == grid.end()) return;

    if (grid[pos].type == Celltype::MOVER) {
        push(pos, d, 0, nullptr);
    } else if (grid[pos].type == Celltype::GENERATOR) {
        Cord2d pos_front = offset(pos, d);
        Cord2d pos_back = offset(pos, reverse(d));
        if (grid.count(pos_back) > 0) {
            if (grid.count(pos_front)) push(pos_front, d, 0, &grid[pos_back]);
            else grid[pos_front] = grid[pos_back];
        }
    } else if (grid[pos].type == Celltype::ROTATOR_CW || grid[pos].type == Celltype::ROTATOR_CCW) {
        int rotation = grid[pos].type == Celltype::ROTATOR_CW ? 1 : -1;
        for (Dir d: dir_set) {
            if (grid.count(offset(pos, d))) {
                grid[offset(pos, d)].dir = rotate_dir(grid[offset(pos, d)].dir, rotation);
            }
        }
    }
}

int get_pos_depth(Cord2d pos, Dir d) {
    return (d == Dir::RIGHT || d == Dir::LEFT) ? pos.x : pos.y;
}

void run_simulation() {
    Celltype update_order[4] = {Celltype::GENERATOR, Celltype::ROTATOR_CW, Celltype::ROTATOR_CCW, Celltype::MOVER};

    for (Celltype c: update_order) {
        for (int d = 0; d < 4; d++) {
            std::map<int, std::vector<Cord2d>> depth;
            for (auto& [pos, cell]: grid) {
                if (grid[pos].dir == (Dir)d && grid[pos].type == c) {
                    depth[get_pos_depth(pos, cell.dir)].push_back(pos);
                }
            }
            bool iter_reverse = (Dir)d == Dir::RIGHT || (Dir)d == Dir::DOWN;
            if (iter_reverse) {
                for (auto iter = depth.rbegin(); iter != depth.rend(); iter++) {
                    for (auto& pos: iter->second) process_cell((Dir)d, pos);
                }
            } else {
                for (auto iter = depth.begin(); iter != depth.end(); iter++) {
                    for (auto& pos: iter->second) process_cell((Dir)d, pos);
                }
            }
        }
    }
}

int main(void) {
    const int screenW = 800;
    const int screenH = 450;

    const float ZOOM_SPEED = 3.0f;
    const float CAM_SPEED = 600.0f;
    const float CELL_SIZE = 40.0f;

    float zoom = 1.0f;

    Vector2 cam = {0, 0};

    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(screenW, screenH, "raylib");

    for (auto& [type, data]: cell_data) {
        Image img = LoadImage(TextFormat("textures/%s", data.filename));
        data.texture = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        Vector2 screen_size = {(float)GetScreenWidth(), (float)GetScreenHeight()};
        Vector2 mouse = GetMousePosition();

        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) cam.y -= CAM_SPEED * dt / zoom;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) cam.y += CAM_SPEED * dt / zoom;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) cam.x -= CAM_SPEED * dt / zoom;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) cam.x += CAM_SPEED * dt / zoom;
        float dt_zoom = 0;
        if (IsKeyDown(KEY_Z)) dt_zoom -= dt * ZOOM_SPEED * zoom;
        if (IsKeyDown(KEY_X)) dt_zoom += dt * ZOOM_SPEED * zoom;
        zoom += dt_zoom;

        zoom = fmin(10.0f, fmax(zoom, 0.1f));

        float cell_render = CELL_SIZE * zoom;
        Vector2 world = {cam.x - screen_size.x / (2*zoom), cam.y - screen_size.y / (2*zoom)};
        Vector2 hover = {(world.x + mouse.x / zoom) / CELL_SIZE, (world.y + mouse.y / zoom) / CELL_SIZE};
        Vector2 offset = {
            (ceil(world.x / CELL_SIZE) * CELL_SIZE - world.x) * zoom,
            (ceil(world.y / CELL_SIZE) * CELL_SIZE - world.y) * zoom
        };

        for (auto& [type, data]: cell_data) {
            if (IsKeyPressed(data.key)) celltype_selected = type;
        }
        
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            Cord2d c = {(int)floorf(hover.x), (int)floorf(hover.y)};
            grid[c] = Cell{celltype_selected, dir_selected};
        } else if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
            Cord2d c = {(int)floorf(hover.x), (int)floorf(hover.y)};
            grid.erase(c);
        }

        if (IsKeyPressed(KEY_Q)) dir_selected = rotate_dir(dir_selected, -1);
        if (IsKeyPressed(KEY_E)) dir_selected = rotate_dir(dir_selected, 1);
        if (IsKeyPressed(KEY_SPACE)) run_simulation();

        BeginDrawing();
        ClearBackground(bg);

        int cnt_x = (int)(screen_size.x / cell_render) + 2;
        int cnt_y = (int)(screen_size.y / cell_render) + 2;
        int base_x = floorf(world.x / CELL_SIZE);
        int base_y = floorf(world.y / CELL_SIZE);
        float x0 = (base_x * CELL_SIZE - world.x) * zoom;
        float y0 = (base_y * CELL_SIZE - world.y) * zoom;

        for (float i = offset.x; i <= screen_size.x; i += cell_render) {
            DrawLineEx((Vector2){i, 0}, (Vector2){i, screen_size.y}, 2, lines);
        }
        for (float i = offset.y; i <= screen_size.y; i += cell_render) {
            DrawLineEx((Vector2){0, i}, (Vector2){screen_size.x, i}, 2, lines);
        }

        for (int i = 0; i <= cnt_x; i++) {
            for (int j = 0; j <= cnt_y; j++) {
                Cord2d c = {base_x + i, base_y + j};
                if (grid.count(c)) {
                    Texture2D texture = cell_data[grid[c].type].texture;
                    Rectangle src = {0, 0, (float)texture.width, (float)texture.height};
                    Vector2 origin = {cell_render / 2, cell_render / 2};
                    Rectangle dest = {x0 + i * cell_render + origin.x, y0 + j * cell_render + origin.y, cell_render, cell_render};
                    DrawTexturePro(texture, src, dest, origin, dir_to_degrees(grid[c].dir), WHITE);
                }
            }
        }

        

        DrawText(TextFormat("cam: (%d, %d)", (int)cam.x, (int)cam.y), 10, 10, 20, WHITE);
        DrawText(TextFormat("hover: (%d, %d)", (int)hover.x, (int)hover.y), 10, 30, 20, YELLOW);
        DrawText(TextFormat("selected cell: %d", celltype_selected), 10, 50, 20, ORANGE);
        DrawText(TextFormat("selected dir: %d", dir_selected), 10, 70, 20, RED);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}