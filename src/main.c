#include "main.h"

#include <math.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "genann.h"
#include "raylib.h"
#include "raymath.h"
#include "util/gui.h"
#include "vec.h"

static void reset_simulation(void);
static void train_ants(double fixed_delta);

#pragma region setup

/**
 * INPUT:
 * 0: angle cos
 * 1: angle sin
 * 2: on spawn
 * 3: spawn delta cos
 * 4: spawn delta sin
 * 5: on food
 * 6: food delta cos
 * 7: food delta sin
 * 8: near food
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
double learning_rate = LEARN_RATE;
bool training = true;
bool reset = false;
bool auto_reset = true;
bool warp = false;

vec_ant_t ant_vec;
vec_food_t food_vec;

const int starting_ants = 100;
const int starting_food = 10;
const int food_radius = 40;
const int food_detection_radius = 400;
const int min_food_distance = 500;
const int max_starting_food_amount = 200;
const int min_starting_food_amount = 50;

double tick_speed = 1.0;

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
  if (warp) {
    if (tick_speed <= 1.1) {
      tick_speed = WARP_SPEED;
    }

    // Scale tick speed based on the render time to get 30 FPS
    tick_speed = fmin(tick_speed * 1.1, fmax(tick_speed / 4, tick_speed / (30.0 * GetFrameTime())));

    tick_speed = fmin(WARP_SPEED * 100, tick_speed);
  } else {
    tick_speed = 1.0f;
  }

  const double fixed_delta = 1.0 / (double)TICK_RATE;
  const double scaled_delta = fixed_delta / tick_speed;
  static double simulated_time = 0.0;
  static bool first = false;
  static double last_time = 0.0;
  static double simulation_time = 0.0;
  static double last_update_time = 0.0;

  if (reset) {
    reset = false;

    first = false;
    simulation_time = 0.0;
    simulated_time = 0.0;

    reset_simulation();
  }

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
    simulated_time += fixed_delta;

    // Reset simulation every RESET_TIME (scaled by tick speed)
    if (simulated_time >= RESET_TIME && auto_reset) {
      reset_simulation();
      simulated_time = 0.0;
    }

    fixed_update(fixed_delta);
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

  Vector2 mouse_pos = GetMousePosition();
  mouse_pos.x = Remap(mouse_pos.x - letterbox.x, 0, window_w - 2 * letterbox.x, 0, SCREEN_W);
  mouse_pos.y = Remap(mouse_pos.y - letterbox.y, 0, window_h - 2 * letterbox.y, 0, SCREEN_H);

  DrawFPS(0, 0);
  if (gui_draw_button(mouse_pos, (Rectangle){10, 20, 300, 20}, "Reset Speed")) {
    warp = false;
  }

  // Button to start runing the simulation from the network
  if (gui_draw_button(mouse_pos, (Rectangle){SCREEN_W - 350, 10, 300, 20}, "Reset Simulation")) {
    reset = true;
  }

  // Checkbox to start runing the simulation from the network
  training = gui_draw_checkbox(mouse_pos, (Vector2){SCREEN_W - 350, 40}, "Train", training);

  // Checkbox to enable/disable auto reset
  auto_reset = gui_draw_checkbox(mouse_pos, (Vector2){SCREEN_W - 350, 65}, "Auto Reset", auto_reset);

  // Checkbox to enable/disable warp
  warp = gui_draw_checkbox(mouse_pos, (Vector2){SCREEN_W - 350, 90}, "Warp", warp);

  gui_draw_label((Vector2){10, 45}, TextFormat("Tick Speed: %.0f", tick_speed));

  EndTextureMode();
}

void initialize() {
  srand(time(NULL));
  // Init genann
  ant_ann = genann_init(ANN_INPUTS, ANN_HIDDEN_LAYERS, ANN_HIDDEN_NODES, ANN_OUTPUTS);

  // Initialize raylib
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_HIGHDPI);
  InitWindow(0, 0, "Ant Matrix");
  SetTargetFPS(TARGET_FPS);

  ant_texture = LoadTexture("assets/ant.png");

  vec_init(&ant_vec);
  vec_init(&food_vec);

  reset_simulation();

  offscreen = LoadRenderTexture(SCREEN_W, SCREEN_H);
  SetTextureFilter(offscreen.texture, TEXTURE_FILTER_BILINEAR);
  const int height = nearest_16_by_9_height(GetScreenWidth());
  resize_window(height * 16 / 9, height);
  // Position the window in the center of the screen
  SetWindowPosition((GetMonitorWidth(GetCurrentMonitor()) - window_w) / 2,
                    (GetMonitorHeight(GetCurrentMonitor()) - window_h) / 2);
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
  learning_rate = fmax(learning_rate * LEARN_RATE_DECAY, 1e-5);
  const double max_distance = hypot(WORLD_W / 2, WORLD_H / 2);
  const double max_food_distance = food_detection_radius + ANT_DETECTOR_RADIUS * ANT_SCALE * 2.0;

  vec_foreach(&ant_vec, ant, i) {
    ant_update_nearest_food(ant);

    // Update the neural network
    double inputs[ANN_INPUTS] = {0};

    const Vector2 spawn_vector = Vector2Subtract(ant->spawn, ant->pos);
    const double spawn_vector_length = Vector2Length(spawn_vector);
    inputs[0] = enc(cos(ant->rotation));
    inputs[1] = enc(sin(ant->rotation));
    inputs[2] = CheckCollisionPointCircle(ant->pos, ant->spawn, ANT_SPAWN_RADIUS) ? 1.0 : 0.0;
    inputs[3] = enc(spawn_vector_length > 1e-9 ? spawn_vector.x / spawn_vector_length : -cos(ant->rotation));
    inputs[4] = enc(spawn_vector_length > 1e-9 ? spawn_vector.y / spawn_vector_length : -sin(ant->rotation));

    if (ant->nearest_food) {
      const Vector2 food_vector = Vector2Subtract(ant->nearest_food->pos, ant->pos);
      const double food_vector_length = Vector2Length(food_vector);
      inputs[5] = CheckCollisionPointCircle(ant->pos, ant->nearest_food->pos, ant->nearest_food->radius) ? 1.0 : 0.0;
      inputs[6] = enc(food_vector_length > 1e-9 ? food_vector.x / food_vector_length : -cos(ant->rotation));
      inputs[7] = enc(food_vector_length > 1e-9 ? food_vector.y / food_vector_length : -sin(ant->rotation));
    } else {
      inputs[5] = 0.0;
      inputs[6] = 0.0;
      inputs[7] = 0.0;
    }

    inputs[8] = ant->nearest_food ? 1.0 : 0.0;
    inputs[9] = ant->has_food ? 1.0 : 0.0;
    inputs[10] = ant->is_coliding ? 1.0 : 0.0;

    if (training) {
      // Quarter chance to train the ant, otherwise run the ant
      if (rand() % 16 == 0) {
        const double *pred = genann_run(ant_ann, inputs);
        ant_logic_t logic = {0};
        const double len = hypot(dec(pred[0]), dec(pred[1]));
        if (len > 0.0) {
          logic.angle = constrain_angle(atan2(dec(pred[1]) / len, dec(pred[0]) / len));
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
        continue;
      }

      ant_logic_t logic = train_update_ant(ant, fixed_delta);

      double outputs[ANN_OUTPUTS] = {0};
      outputs[0] = enc(cos(logic.angle));
      outputs[1] = enc(sin(logic.angle));
      outputs[2] = logic.action == ANT_STEP_ACTION ? 1.0 : 0.0;
      outputs[3] = logic.action == ANT_GATHER_ACTION ? 1.0 : 0.0;
      outputs[4] = logic.action == ANT_DROP_ACTION ? 1.0 : 0.0;

      if (logic.action != ANT_STEP_ACTION) {
        for (int j = 0; j < 50; j++) {
          genann_train(ant_ann, inputs, outputs, learning_rate);
        }
      } else if (inputs[10] > 0) {
        for (int j = 0; j < 10; j++) {
          genann_train(ant_ann, inputs, outputs, learning_rate);
        }
      } else if (inputs[9] > 0) {
        for (int j = 0; j < 1; j++) {
          genann_train(ant_ann, inputs, outputs, learning_rate);
        }
      } else {
        genann_train(ant_ann, inputs, outputs, learning_rate * 0.2);
      }

      if (!warp && rand() % 1000 == 0) {
        const double *pred = genann_run(ant_ann, inputs);
        printf("Inputs: ");
        for (int j = 0; j < ANN_INPUTS; j++) {
          printf("%.3f ", inputs[j]);
        }
        printf("\nOutputs: ");
        for (int j = 0; j < ANN_OUTPUTS; j++) {
          printf("%.3f ", outputs[j]);
        }
        printf("\nPred: ");
        for (int j = 0; j < ANN_OUTPUTS; j++) {
          printf("%.3f ", pred[j]);
        }
        printf("\n");
      }

    } else {
      const double *pred = genann_run(ant_ann, inputs);
      ant_logic_t logic = {0};
      const double len = hypot(dec(pred[0]), dec(pred[1]));
      if (len > 0.0) {
        logic.angle = constrain_angle(atan2(dec(pred[1]) / len, dec(pred[0]) / len));
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
      if (rand() % 1000 == 0) {
        printf("Inputs: ");
        for (int j = 0; j < ANN_INPUTS; j++) {
          printf("%.3f ", inputs[j]);
        }
        printf("\nPred: ");
        for (int j = 0; j < ANN_OUTPUTS; j++) {
          printf("%.3f ", pred[j]);
        }
        printf("\n");
      }
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