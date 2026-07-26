#include "raylib.h"
#include "math.h"
#include <cstdint>
#include <unordered_map>

const Color bg = {28, 30, 36, 255};
const Color lines = {42, 46, 54, 255};
const Color hl_fill = {70, 78, 96, 90 };
const Color hl_edge = {110, 124, 150, 255};

enum class Celltype: uint8_t {MOVER, PUSH};
enum class Dir: uint8_t {UP, RIGHT, LEFT, DOWN};
struct Cell {
    Celltype type;
    Dir dir = Dir::UP;
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

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        Vector2 screen_size = {(float)GetScreenWidth(), (float)GetScreenHeight()};
        Vector2 mouse = GetMousePosition();

        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) cam.y -= CAM_SPEED * dt / zoom;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) cam.y += CAM_SPEED * dt / zoom;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) cam.x -= CAM_SPEED * dt / zoom;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) cam.x += CAM_SPEED * dt / zoom;

        if (IsKeyDown(KEY_Q)) zoom -= dt * ZOOM_SPEED * zoom;
        if (IsKeyDown(KEY_E)) zoom += dt * ZOOM_SPEED * zoom;

        zoom = fmin(10.0f, fmax(zoom, 0.1f));

        float cell_render = CELL_SIZE * zoom;
        Vector2 world = {cam.x - screen_size.x / (2*zoom), cam.y - screen_size.y / (2*zoom)};
        Vector2 hover = {(world.x + mouse.x / zoom) / CELL_SIZE, (world.y + mouse.y / zoom) / CELL_SIZE};
        Vector2 offset = {
            (ceil(world.x / CELL_SIZE) * CELL_SIZE - world.x) * zoom,
            (ceil(world.y / CELL_SIZE) * CELL_SIZE - world.y) * zoom
        };

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Cord2d c = {(int)floorf(hover.x), (int)floorf(hover.y)};
            grid[c] = Cell{Celltype::MOVER, Dir::UP};
        }

        BeginDrawing();
        ClearBackground(bg);

        int cnt_x = (int)(screen_size.x / cell_render) + 2;
        int cnt_y = (int)(screen_size.y / cell_render) + 2;
        int base_x = floorf(world.x / CELL_SIZE);
        int base_y = floorf(world.y / CELL_SIZE);
        float x0 = (base_x * CELL_SIZE - world.x) * zoom;
        float y0 = (base_y * CELL_SIZE - world.y) * zoom;
        // = {(int)(world.x / CELL_SIZE), (int)(world.y / CELL_SIZE)};

        for (int i = 0; i <= cnt_x; i++) {
            for (int j = 0; j <= cnt_y; j++) {
                Cord2d c = {base_x + i, base_y + j};
                if (grid.count(c)) {
                    Vector2 c_render = {offset.x + i * cell_render, offset.y + j * cell_render};
                    DrawRectangle(x0 + i * cell_render, y0 + j * cell_render, cell_render, cell_render, RED);
                }
            }
        }

        for (float i = offset.x; i <= screen_size.x; i += cell_render) {
            DrawLineEx((Vector2){i, 0}, (Vector2){i, screen_size.y}, 2, lines);
        }
        for (float i = offset.y; i <= screen_size.y; i += cell_render) {
            DrawLineEx((Vector2){0, i}, (Vector2){screen_size.x, i}, 2, lines);
        }

        DrawText(TextFormat("cam: (%d, %d)", (int)cam.x, (int)cam.y), 10, 10, 20, WHITE);
        DrawText(TextFormat("hover: (%d, %d)", (int)hover.x, (int)hover.y), 10, 30, 20, YELLOW);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
