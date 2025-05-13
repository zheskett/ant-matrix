#include "raylib.h"
#include <math.h>

void render(void);
void render_present(void);
void resize_window(int w, int h);

// Initialization
int window_w = 1920;
int window_h = 1080;
const int screen_w = 1280;
const int screen_h = 720;
RenderTexture2D offscreen;
Rectangle letterbox = { 0, 0, 1920, 1080 };

int main() {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
  InitWindow(window_w, window_h, "raylib [core] example - basic window");
  offscreen = LoadRenderTexture(screen_w, screen_h);

  SetTargetFPS(60);
  resize_window(window_w, window_h);
  while (!WindowShouldClose()) {
    if (IsWindowResized()) {
      resize_window(GetScreenWidth(), GetScreenHeight());
    }

    render();
    render_present();
  }

  CloseWindow();

  return 0;
}

void resize_window(int w, int h) {
  SetWindowSize(w, h);
  window_w = w;
  window_h = h;

  letterbox.width = screen_w;
  letterbox.height = screen_h;

  float ratio_x = window_w / (float) screen_w;
  float ratio_y = window_h / (float) screen_h;
  float ratio = fminf(ratio_x, ratio_y);
  float offset_x = (window_w - ratio * screen_w) * 0.5f;
  float offset_y = (window_h - ratio * screen_h) * 0.5f;
  letterbox = (Rectangle){ offset_x, offset_y,
      ratio * screen_w, ratio * screen_h };
}

void render(void) {
  Camera2D cam = {
      .target = (Vector2){0, 0},
      .offset = (Vector2){0, 0},
      .rotation = 0.0f,
      .zoom = 1.0f
  };

  BeginTextureMode(offscreen);
  BeginMode2D(cam);
  ClearBackground(BROWN);

  DrawText("This is a test", 100, 100, 20, WHITE);
  DrawFPS(0, 0);

  EndMode2D();
  EndTextureMode();
}

void render_present(void) {
  /* render offscreen to display */

  BeginDrawing();
  ClearBackground(BROWN);

  const Rectangle render_src = { 0, 0, (float) screen_w, -(float) screen_h };
  const Vector2 render_origin = { 0, 0 };
  DrawTexturePro(offscreen.texture, render_src, letterbox, render_origin, 0.0f, WHITE);
  EndDrawing();
}
