#include "raylib.h"
#include "math.h"

const Color bg = {28, 30, 36, 255};
const Color lines = {42, 46, 54, 255};
const Color hl_fill = {70, 78, 96, 90 };
const Color hl_edge = {110, 124, 150, 255};

int main(void) {
    const int screenW = 800;
    const int screenH = 450;

    const float ZOOM_SPEED = 3.0f;
    const float CAM_SPEED = 400.0f;
    const float CELL_SIZE = 40.0f;

    float zoom = 1.0f;

    Vector2 cam = {0, 0};

    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(screenW, screenH, "raylib");

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        Vector2 screen_size = {GetScreenWidth(), GetScreenHeight()};
        Vector2 mouse = GetMousePosition();
        
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) cam.y -= CAM_SPEED * dt / zoom;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) cam.y += CAM_SPEED * dt / zoom;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) cam.x -= CAM_SPEED * dt / zoom;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) cam.x += CAM_SPEED * dt / zoom;

        if (IsKeyDown(KEY_Q)) zoom -= dt * ZOOM_SPEED * zoom;
        if (IsKeyDown(KEY_E)) zoom += dt * ZOOM_SPEED * zoom;

        zoom = fmin(10.0, fmax(zoom, 0.1));
        
        float cell_render = CELL_SIZE * zoom;
        Vector2 screen_start = {cam.x - screen_size.x / (2*zoom), cam.y - screen_size.y / (2*zoom)};

        Vector2 offset = {
            (ceil(screen_start.x / CELL_SIZE) * CELL_SIZE - screen_start.x) * zoom,
            (ceil(screen_start.y / CELL_SIZE) * CELL_SIZE - screen_start.y) * zoom
        };

        BeginDrawing();
        ClearBackground(bg);

        for (float i = offset.x; i <= screen_size.x; i += cell_render) {
            DrawLineEx((Vector2){i, 0}, (Vector2){i, screen_size.y}, 2, lines);
        }
        for (float i = offset.y; i <= screen_size.y; i += cell_render) {
            DrawLineEx((Vector2){0, i}, (Vector2){screen_size.x, i}, 2, lines);
        }

        DrawText(TextFormat("(%d, %d)", (int)cam.x, (int)cam.y), 10, 10, 20, WHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
