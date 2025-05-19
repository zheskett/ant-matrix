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

#pragma region setup

const int starting_ants = 10;
const int starting_food = 4;
const int food_radius = 30;
const int food_detection_radius = 120;
const int min_food_distance = 500;
const int max_starting_food_amount = 80;
const int min_starting_food_amount = 20;
double tick_speed = 1;

int window_w = 1920;
int window_h = 1080;

float zoom = 1.0f;
Vector2 target = {WORLD_W / 2.0f, WORLD_H / 2.0f};

input_t input_map = {(Vector2){0, 0}, 0};
Vector2 spawn = {0, 0};

RenderTexture2D offscreen;
Texture2D ant_texture;
Rectangle letterbox = {0, 0, SCREEN_W, SCREEN_H};
vec_ant_t ant_vec;
vec_food_t food_vec;

#pragma endregion

int main() {
  initialize();

  while (!WindowShouldClose()) {
    if (IsWindowResized()) {
      resize_window(GetScreenWidth(), GetScreenHeight());
    }

    input();
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

void input() {
  input_map.dir = (Vector2){0, 0};
  if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
    input_map.dir.x -= 1;
  }
  if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
    input_map.dir.x += 1;
  }
  if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
    input_map.dir.y -= 1;
  }
  if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
    input_map.dir.y += 1;
  }

  input_map.zoom += GetMouseWheelMove();
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

  target.x += input_map.dir.x * CAM_SPEED * delta_time;
  target.y += input_map.dir.y * CAM_SPEED * delta_time;
  zoom += input_map.zoom * 0.1f;
  input_map.zoom = 0;
  zoom = fmaxf(1.0f / WORLD_SCALE, fminf(zoom, 2.0f));
  zoom = roundf(zoom * 100.0f) / 100.0f;

  // Clamp target to world bounds with respect to zoom
  target.x = fmaxf((SCREEN_W / 2.0f) * (WORLD_SCALE / (WORLD_SCALE * zoom)),
                   fminf(WORLD_W - ((SCREEN_W / 2.0f) * (WORLD_SCALE / (WORLD_SCALE * zoom))), target.x));
  target.y = fmaxf((SCREEN_H / 2.0f) * (WORLD_SCALE / (WORLD_SCALE * zoom)),
                   fminf(WORLD_H - ((SCREEN_H / 2.0f) * (WORLD_SCALE / (WORLD_SCALE * zoom))), target.y));
}

void fixed_update(double fixed_delta) {
  ant_t *ant = NULL;
  food_t *food = NULL;
  int i = 0;
  vec_foreach(&ant_vec, ant, i) { update_ant(ant, fixed_delta); }
  vec_foreach(&food_vec, food, i) { update_food(food, fixed_delta); }
}

void render() {
  Camera2D cam = {0};
  cam.target = target;
  cam.offset = (Vector2){SCREEN_W / 2.0f, SCREEN_H / 2.0f};
  cam.rotation = 0.0f;
  cam.zoom = zoom;

  BeginTextureMode(offscreen);
  BeginMode2D(cam);
  ClearBackground(BROWN);

  // Draw center lines
  DrawLine(WORLD_W / 2, 0, WORLD_W / 2, WORLD_H, DARKGRAY);
  DrawLine(0, WORLD_H / 2, WORLD_W, WORLD_H / 2, DARKGRAY);

  // Draw ants
  ant_t *ant = NULL;
  food_t *food = NULL;
  int i = 0;

  vec_foreach(&ant_vec, ant, i) { draw_ant(ant); }
  vec_foreach(&food_vec, food, i) { draw_food(food); }

  // Draw in top left corner on screen regardless of camera
  EndMode2D();

  DrawFPS(0, 0);
  EndTextureMode();
}

void initialize() {
  srand(time(NULL));
  spawn = (Vector2){rand() % WORLD_W, rand() % WORLD_H};

  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
  InitWindow(window_w, window_h, "Ant Matrix");
  SetTargetFPS(TARGET_FPS);

  vec_init(&ant_vec);
  ant_texture = LoadTexture("assets/ant.png");
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
      food_pos.x = rand() % WORLD_W;
      food_pos.y = rand() % WORLD_H;
    }
    vec_push(&food_vec,
             create_food(food_pos, food_radius, food_detection_radius,
                         rand() % (max_starting_food_amount - min_starting_food_amount) + min_starting_food_amount));
  }

  offscreen = LoadRenderTexture(SCREEN_W, SCREEN_H);

  resize_window(window_w, window_h);
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
  ClearBackground(BLACK);

  const Rectangle render_src = {0, 0, (float)SCREEN_W, -(float)SCREEN_H};
  const Vector2 render_origin = {0, 0};
  DrawTexturePro(offscreen.texture, render_src, letterbox, render_origin, 0.0f, WHITE);
  EndDrawing();
}
