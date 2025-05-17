#include "main.h"

#include <math.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "entities/ant.h"
#include "entities/food.h"
#include "raylib.h"
#include "raymath.h"
#include "util/definitions.h"
#include "vec.h"

#pragma region initialization

const int starting_ants = 10;
const int starting_food = 4;
const int food_radius = 30;
const int food_detection_radius = 120;
const int min_food_distance = 500;
const int max_starting_food_amount = 80;
const int min_starting_food_amount = 20;
double tick_speed = 1.0;

int window_w = 1920;
int window_h = 1080;
Vector2 spawn = {0, 0};

RenderTexture2D offscreen;
Rectangle letterbox = {0, 0, SCREEN_W, SCREEN_H};
vec_ant_t ant_vec;
vec_food_t food_vec;

#pragma endregion

int main() {
  srand(time(NULL));
  spawn = (Vector2){rand() % SCREEN_W, rand() % SCREEN_H};

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
  InitWindow(window_w, window_h, "Ant Matrix");
  SetTargetFPS(TARGET_FPS);

  vec_init(&ant_vec);
  Texture2D ant_texture = LoadTexture("assets/ant.png");
  SetTextureFilter(ant_texture, TEXTURE_FILTER_BILINEAR);

  // Create ants
  for (int i = 0; i < starting_ants; i++) {
    vec_push(&ant_vec, create_ant(spawn, &ant_texture, rand() % 360));
  }

  // Create food away from ants
  vec_init(&food_vec);
  for (int i = 0; i < starting_food; i++) {
    Vector2 food_pos = spawn;
    while (Vector2Distance(spawn, food_pos) < min_food_distance) {
      food_pos.x = rand() % SCREEN_W;
      food_pos.y = rand() % SCREEN_H;
    }
    vec_push(&food_vec,
             create_food(food_pos, food_radius, food_detection_radius,
                         rand() % (max_starting_food_amount - min_starting_food_amount) + min_starting_food_amount));
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
  food_t *food = NULL;
  int i = 0;
  vec_foreach(&ant_vec, ant, i) { update_ant(ant, fixed_delta); }
  vec_foreach(&food_vec, food, i) { update_food(food, fixed_delta); }
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
  food_t *food = NULL;
  int i = 0;

  vec_foreach(&ant_vec, ant, i) { draw_ant(ant); }
  vec_foreach(&food_vec, food, i) { draw_food(food); }

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
