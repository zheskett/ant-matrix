#include "main.h"

#include <math.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "entities/ant.h"
#include "raylib.h"
#include "util/definitions.h"
#include "vec.h"

// Initialization
double tick_speed = 1.0;

int window_w = 1920;
int window_h = 1080;

RenderTexture2D offscreen;
Rectangle letterbox = {0, 0, SCREEN_W, SCREEN_H};
vec_ant_t ant_vec;

int main() {
  srand(time(NULL));

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
  InitWindow(window_w, window_h, "Ant Matrix");
  SetTargetFPS(TARGET_FPS);

  vec_init(&ant_vec);
  Texture2D ant_texture = LoadTexture("assets/ant.png");
  SetTextureFilter(ant_texture, TEXTURE_FILTER_BILINEAR);

  // Create ants
  for (int i = 0; i < 500; i++) {
    vec_push(&ant_vec, create_ant(rand() % SCREEN_W, rand() % SCREEN_H, &ant_texture, rand() % 360));
  }

  offscreen = LoadRenderTexture(SCREEN_W, SCREEN_H);

  resize_window(window_w, window_h);

  while (!WindowShouldClose()) {
    if (IsWindowResized()) {
      resize_window(GetScreenWidth(), GetScreenHeight());
    }

    update();
    render();
    render_present();
  }

  UnloadRenderTexture(offscreen);
  ant_t *ant = NULL;
  int i = 0;
  vec_foreach(&ant_vec, ant, i) { destroy_ant(ant); }
  vec_deinit(&ant_vec);
  UnloadTexture(ant_texture);
  CloseWindow();

  return 0;
}

void update() {
  double fixed_delta = 1.0 / (double)TICK_RATE;
  double scaled_delta = fixed_delta / tick_speed;
  static bool first = false;
  static double last_time = 0.0;
  static double simulation_time = 0.0;
  static double last_update_time = 0.0;

  if (!first) {
    last_time = GetTime();
    last_update_time = last_time;
    first = true;
  }
  double current_time = GetTime();
  double delta_time = fmin(current_time - last_time, MAX_DELTA);

  last_time = current_time;
  simulation_time += delta_time;

  while (simulation_time >= scaled_delta) {
    simulation_time -= scaled_delta;

    fixed_update(fixed_delta);
  }
}

void fixed_update(double fixed_delta) {
  ant_t *ant = NULL;
  int i = 0;
  vec_foreach(&ant_vec, ant, i) { update_ant(ant, fixed_delta); }
}

void render() {
  Camera2D cam = {.target = (Vector2){0, 0}, .offset = (Vector2){0, 0}, .rotation = 0.0f, .zoom = 1.0f};

  BeginTextureMode(offscreen);
  BeginMode2D(cam);
  ClearBackground(BROWN);

  // Draw center lines
  DrawLine(SCREEN_W / 2, 0, SCREEN_W / 2, SCREEN_H, DARKGRAY);
  DrawLine(0, SCREEN_H / 2, SCREEN_W, SCREEN_H / 2, DARKGRAY);

  // Draw ants
  ant_t *ant = NULL;
  int i = 0;

  vec_foreach(&ant_vec, ant, i) { draw_ant(ant); }

  DrawFPS(0, 0);

  EndMode2D();
  EndTextureMode();
}

void resize_window(int w, int h) {
  SetWindowSize(w, h);
  window_w = w;
  window_h = h;

  letterbox.width = SCREEN_W;
  letterbox.height = SCREEN_H;

  float ratio_x = window_w / (float)SCREEN_W;
  float ratio_y = window_h / (float)SCREEN_H;
  float ratio = fminf(ratio_x, ratio_y);
  float offset_x = (window_w - ratio * SCREEN_W) * 0.5f;
  float offset_y = (window_h - ratio * SCREEN_H) * 0.5f;
  letterbox = (Rectangle){offset_x, offset_y, ratio * SCREEN_W, ratio * SCREEN_H};
}

void render_present() {
  /* render offscreen to display */

  BeginDrawing();
  ClearBackground(BROWN);

  const Rectangle render_src = {0, 0, (float)SCREEN_W, -(float)SCREEN_H};
  const Vector2 render_origin = {0, 0};
  DrawTexturePro(offscreen.texture, render_src, letterbox, render_origin, 0.0f, WHITE);
  EndDrawing();
}
