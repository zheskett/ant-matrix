#include "main.h"

#include <math.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "entities/ant.h"
#include "entities/food.h"
#include "genann.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "raylib.h"
#include "raymath.h"
#include "util/definitions.h"
#include "vec.h"

static void reset_simulation(void);
static void train_ants(double fixed_delta);

#pragma region setup

/**
 * INPUT:
 * 0: rotation cos
 * 1: rotation sin
 * 2: spawn cos
 * 3: spawn sin
 * 4: spawn dist
 * 5: food cos
 * 6: food sin
 * 7: food dist
 * 9: near food
 * 9: has food
 * 10: is colliding
 *
 * OUTPUT:
 * 0: angle cos
 * 1: angle sin
 * 2: step
 * 3: gather
 * 4: drop
 */
genann *ant_ann = NULL;
bool training = true;
bool reset = false;
bool auto_reset = true;
bool warp = false;
double start_time = 0.0;

vec_ant_t ant_vec;
vec_food_t food_vec;

const int starting_ants = 100;
const int starting_food = 10;
const int food_radius = 50;
const int food_detection_radius = 500;
const int min_food_distance = 500;
const int max_starting_food_amount = 30;
const int min_starting_food_amount = 10;
float tick_speed = 1.0;

int window_w = 1920;
int window_h = 1080;

float zoom = 1.0f;
Vector2 target = {WORLD_W / 2.0f, WORLD_H / 2.0f};
Vector2 prev_mouse_pos = {0, 0};
const Vector2 spawn = {WORLD_W / 2, WORLD_H / 2};

RenderTexture2D offscreen;
Texture2D ant_texture;
Rectangle letterbox = {0, 0, SCREEN_W, SCREEN_H};

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
  UnloadTexture(ant_texture);

  int i = 0;
  ant_t *ant = NULL;
  vec_foreach(&ant_vec, ant, i) { destroy_ant(ant); }
  vec_deinit(&ant_vec);

  food_t *food = NULL;
  vec_foreach(&food_vec, food, i) { destroy_food(food); }
  vec_deinit(&food_vec);
  genann_free(ant_ann);

  CloseWindow();

  return 0;
}

void input() {
  Vector2 dir = (Vector2){0, 0};
  Vector2 mouse_delta = (Vector2){0, 0};
  float zoom_delta = 0;
  Vector2 mouse_pos = GetMousePosition();
  mouse_pos.x = Remap(mouse_pos.x - letterbox.x, 0, window_w - 2 * letterbox.x, 0, SCREEN_W);
  mouse_pos.y = Remap(mouse_pos.y - letterbox.y, 0, window_h - 2 * letterbox.y, 0, SCREEN_H);
  mouse_pos = Vector2Scale(mouse_pos, WORLD_SCALE / (WORLD_SCALE * zoom));

  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    mouse_delta = Vector2Subtract(prev_mouse_pos, mouse_pos);
  }

  if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
    dir.x -= 1;
  }
  if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
    dir.x += 1;
  }
  if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
    dir.y -= 1;
  }
  if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
    dir.y += 1;
  }

  zoom_delta = GetMouseWheelMove();
  if (zoom_delta != 0) {
    Vector2 mouse_world_before_pos =
        Vector2Add(mouse_pos, Vector2Subtract(target, Vector2Scale((Vector2){SCREEN_W / 2.0f, SCREEN_H / 2.0f},
                                                                   WORLD_SCALE / (WORLD_SCALE * zoom))));

    float prev_zoom = zoom;
    zoom += zoom_delta * 0.1f;
    zoom = fmaxf(1.0f / WORLD_SCALE, fminf(zoom, 2.0f));
    zoom = roundf(zoom * 100.0f) / 100.0f;

    Vector2 mouse_after_pos = GetMousePosition();
    mouse_after_pos.x = Remap(mouse_after_pos.x - letterbox.x, 0, window_w - 2 * letterbox.x, 0, SCREEN_W);
    mouse_after_pos.y = Remap(mouse_after_pos.y - letterbox.y, 0, window_h - 2 * letterbox.y, 0, SCREEN_H);
    mouse_after_pos = Vector2Scale(mouse_after_pos, WORLD_SCALE / (WORLD_SCALE * zoom));
    const Vector2 mouse_world_after_pos =
        Vector2Add(mouse_after_pos, Vector2Subtract(target, Vector2Scale((Vector2){SCREEN_W / 2.0f, SCREEN_H / 2.0f},
                                                                         WORLD_SCALE / (WORLD_SCALE * zoom))));

    const Vector2 world_delta = Vector2Subtract(mouse_world_before_pos, mouse_world_after_pos);
    target = Vector2Add(target, world_delta);
  }

  target = Vector2Add(target, Vector2Scale(dir, CAM_SPEED * GetFrameTime()));
  target = Vector2Add(target, mouse_delta);
  // Clamp target to world bounds with respect to zoom
  target.x = fmaxf((SCREEN_W / 2.0f) * (WORLD_SCALE / (WORLD_SCALE * zoom)),
                   fminf(WORLD_W - ((SCREEN_W / 2.0f) * (WORLD_SCALE / (WORLD_SCALE * zoom))), target.x));
  target.y = fmaxf((SCREEN_H / 2.0f) * (WORLD_SCALE / (WORLD_SCALE * zoom)),
                   fminf(WORLD_H - ((SCREEN_H / 2.0f) * (WORLD_SCALE / (WORLD_SCALE * zoom))), target.y));

  prev_mouse_pos = mouse_pos;
}

void update() {
  const double fixed_delta = 1.0 / (double)TICK_RATE;
  const double scaled_delta = fixed_delta / tick_speed;
  static bool first = false;
  static double last_time = 0.0;
  static double simulation_time = 0.0;
  static double last_update_time = 0.0;

  if (reset) {
    reset = false;

    first = false;
    simulation_time = 0.0;

    reset_simulation();
  }

  if (!first) {
    last_time = GetTime();
    last_update_time = last_time;
    start_time = last_time;
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

  // Reset simulation every 2 minutes (scaled by tick speed)
  if ((current_time - start_time) * tick_speed >= 120.0 && auto_reset) {
    reset = true;
  }
}

void fixed_update(double fixed_delta) {
  train_ants(fixed_delta);

  food_t *food = NULL;
  int i = 0;
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

  DrawCircleV(spawn, ANT_SPAWN_RADIUS, DARKBLUE);

  ant_t *ant = NULL;
  food_t *food = NULL;
  int i = 0;
  vec_foreach(&food_vec, food, i) { draw_food(food); }
  vec_foreach(&ant_vec, ant, i) { draw_ant(ant); }

  EndMode2D();

  DrawFPS(0, 0);
  GuiSlider((Rectangle){15, 20, 300, 20}, "0", "5", &tick_speed, 0.0f, 5.0f);
  if (GuiButton((Rectangle){15, 60, 300, 20}, "Reset Speed")) {
    warp = false;
    tick_speed = 1.0f;
  }

  // Button to start runing the simulation from the network
  if (GuiButton((Rectangle){SCREEN_W - 350, 10, 300, 20}, "Reset Simulation")) {
    reset = true;
  }

  // Checkbox to start runing the simulation from the network
  GuiCheckBox((Rectangle){SCREEN_W - 350, 40, 20, 20}, "Train", &training);

  // Checkbox to enable/disable auto reset
  GuiCheckBox((Rectangle){SCREEN_W - 350, 60, 20, 20}, "Auto Reset", &auto_reset);

  // Checkbox to enable/disable warp
  GuiCheckBox((Rectangle){SCREEN_W - 350, 80, 20, 20}, "Warp", &warp);
  if (warp) {
    tick_speed = WARP_SPEED;
  }
  GuiLabel((Rectangle){15, 40, 300, 20}, TextFormat("Tick Speed: %.2f", tick_speed));

  EndTextureMode();
}

void initialize() {
  srand(time(NULL));
  // Init genann
  ant_ann = genann_init(ANN_INPUTS, 3, (int)(ANN_INPUTS * 2), ANN_OUTPUTS);

  // Initialize raylib
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN);
  InitWindow(window_w, window_h, "Ant Matrix");
  SetTargetFPS(TARGET_FPS);

  ant_texture = LoadTexture("assets/ant.png");

  vec_init(&ant_vec);
  vec_init(&food_vec);

  reset_simulation();

  offscreen = LoadRenderTexture(SCREEN_W, SCREEN_H);
  SetTextureFilter(offscreen.texture, TEXTURE_FILTER_BILINEAR);
  resize_window(window_w, window_h);

  // Init GUI
  GuiLoadStyleDefault();
  GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
  GuiSetStyle(SLIDER, TEXT_COLOR_NORMAL, 0xFFFFFFFF);
  GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, 0xFFFFFFFF);
  GuiSetStyle(CHECKBOX, TEXT_COLOR_NORMAL, 0xFFFFFFFF);
  GuiSetStyle(CHECKBOX, TEXT_COLOR_FOCUSED, 0xAAAAAA);
}

void resize_window(int w, int h) {
  SetWindowSize(w, h);
  window_w = w;
  window_h = h;

  letterbox.width = SCREEN_W;
  letterbox.height = SCREEN_H;

  const float ratio_x = window_w / (float)SCREEN_W;
  const float ratio_y = window_h / (float)SCREEN_H;
  const float ratio = fminf(ratio_x, ratio_y);
  const float offset_x = (window_w - ratio * SCREEN_W) * 0.5f;
  const float offset_y = (window_h - ratio * SCREEN_H) * 0.5f;
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

static void train_ants(double fixed_delta) {
  ant_t *ant = NULL;
  int i = 0;

  vec_foreach(&ant_vec, ant, i) {
    ant_update_nearest_food(ant);
    const double max_distance = hypotf(WORLD_W, WORLD_H);

    // Update the neural network
    double inputs[ANN_INPUTS] = {0};
    inputs[0] = enc(cosf(ant->rotation));
    inputs[1] = enc(sinf(ant->rotation));

    const Vector2 spawn_vector = Vector2Subtract(ant->spawn, ant->pos);
    const double spawn_vector_length = Vector2Length(spawn_vector);
    inputs[2] = spawn_vector_length / max_distance;
    inputs[3] = enc(spawn_vector.x / spawn_vector_length);
    inputs[4] = enc(spawn_vector.y / spawn_vector_length);

    if (ant->nearest_food) {
      const Vector2 food_vector = Vector2Subtract(ant->nearest_food->pos, ant->pos);
      const double food_vector_length = Vector2Length(food_vector);
      inputs[5] = food_vector_length / max_distance;
      inputs[6] = enc(food_vector.x / food_vector_length);
      inputs[7] = enc(food_vector.y / food_vector_length);
    } else {
      inputs[5] = 0.0;
      inputs[6] = 0.0;
      inputs[7] = 0.0;
    }
    inputs[8] = ant->nearest_food ? 1.0 : 0.0;
    inputs[9] = ant->has_food ? 1.0 : 0.0;
    inputs[10] = ant->is_coliding ? 1.0 : 0.0;

    if (training) {
      ant_logic_t logic = train_update_ant(ant, fixed_delta);

      double outputs[ANN_OUTPUTS] = {0};
      outputs[0] = enc(cosf(logic.angle));
      outputs[1] = enc(sinf(logic.angle));
      outputs[2] = logic.action == ANT_STEP_ACTION ? 0.8 : 0.0;
      outputs[3] = logic.action == ANT_GATHER_ACTION ? 1.0 : 0.2;
      outputs[4] = logic.action == ANT_DROP_ACTION ? 1.0 : 0.2;

      const double *pred = genann_run(ant_ann, inputs);
      double pred_norm[ANN_OUTPUTS] = {0};
      memcpy(pred_norm, pred, sizeof(double) * ANN_OUTPUTS);

      double len = hypot(dec(pred[0]), dec(pred[1]));
      if (len > 0.0) {
        pred_norm[0] = enc(dec(pred[0]) / len);
        pred_norm[1] = enc(dec(pred[1]) / len);
      } else {
        pred_norm[0] = 0.0;
        pred_norm[1] = 0.0;
      }

      genann_train(ant_ann, inputs, outputs, LEARN_RATE);
      genann_train(ant_ann, inputs, pred_norm, LEARN_RATE * 0.1);

      if (rand() % 100000 == 0) {
        double delta = 0.0;
        for (int i = 0; i < ANN_OUTPUTS; i++) {
          delta += fabs(outputs[i] - pred[i]);
        }
        delta /= ANN_OUTPUTS;
        printf("Delta: %.2f\n", delta);
        printf("Inputs: ");
        for (int j = 0; j < ANN_INPUTS; j++) {
          printf("%.2f ", inputs[j]);
        }
        printf("\nOutputs: ");
        for (int j = 0; j < ANN_OUTPUTS; j++) {
          printf("%.2f ", outputs[j]);
        }
        printf("\nPred: ");
        for (int j = 0; j < ANN_OUTPUTS; j++) {
          printf("%.2f ", pred[j]);
        }
        printf("\n");
      }

    } else {
      const double *pred = genann_run(ant_ann, inputs);
      ant_logic_t logic = {0};
      const double len = hypot(dec(pred[0]), dec(pred[1]));
      if (len > 0.0) {
        logic.angle = constrain_angle(atan2f(dec(pred[1]) / len, dec(pred[0]) / len));
      } else {
        logic.angle = 0.0;
      }
      int choice = 2;
      logic.action = ANT_STEP_ACTION;
      if (pred[3] > pred[choice]) {
        logic.action = ANT_GATHER_ACTION;
        choice = 3;
      }
      if (pred[4] > pred[choice]) {
        logic.action = ANT_DROP_ACTION;
        choice = 4;
      }

      run_update_ant(ant, logic, fixed_delta);
    }
  }
}

static void reset_simulation() {
  ant_t *ant = NULL;
  food_t *food = NULL;
  int i = 0;

  // Destroy all ants and food
  vec_foreach(&ant_vec, ant, i) { destroy_ant(ant); }
  vec_foreach(&food_vec, food, i) { destroy_food(food); }
  vec_clear(&ant_vec);
  vec_clear(&food_vec);

  // Create ants
  for (int i = 0; i < starting_ants; i++) {
    vec_push(&ant_vec, create_ant(spawn, &ant_texture, (float)(rand() % 360) * DEG2RAD));
  }

  // Create food away from ants
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
}