#include "main/simulation.h"

#include <time.h>

#include "neural/nn.h"
#include "raylib.h"
#include "raymath.h"
#include "util/gui.h"

static void reset_simulation(void);
static void train_ants(double fixed_delta);
static neural_network_t *create_ant_net();
static void network_train_step(ant_t *ant, const double *inputs, const double *outputs);

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
 *
 * OUTPUT:
 * 0: change in angle cos
 * 1: change in angle sin
 * 2: step
 * 3: gather
 * 4: drop
 */
neural_network_t *ant_network = NULL;
long double learning_rate = LEARN_RATE;
bool training = true;
bool reset = false;
bool auto_reset = true;
bool warp = false;
bool random_session = false;

double tick_speed = 1.0;
double frame_time_avg = 0.03333333333;

dyn_arr_dbl_t input_list;
dyn_arr_dbl_t output_list;

ant_t *ant_data = NULL;
dyn_arr_ant_t ant_list;
dyn_arr_food_t food_list;

int epoch = 0;
const int starting_ants = 200;
const int starting_food = 10;
const int max_starting_food_amount = 200;
const int min_starting_food_amount = 50;
const double food_radius = 40;
const double food_detection_radius = 300;
const double min_food_distance = 400;
const vector2d_t spawn = {WORLD_W / 2, WORLD_H / 2};

int window_w = 1920;
int window_h = 1080;
float zoom = 1.0f;
Vector2 target = {WORLD_W / 2.0f, WORLD_H / 2.0f};
Vector2 prev_mouse_pos = {0, 0};
RenderTexture2D offscreen;
Texture2D ant_texture;
Rectangle letterbox = {0, 0, SCREEN_W, SCREEN_H};

#pragma endregion

int start(int argc, char **argv) {
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

  if (ant_data) {
    for (int i = 0; i < ant_list.length; i++) {
      ant_t *ant = dyn_arr_get(ant_list, i);
      if (PER_ANT_NETWORK) {
        free_neural_network(ant->net);
      }
    }
    free(ant_data);
  }
  dyn_arr_free(ant_list);

  for (int i = 0; i < food_list.length; i++) {
    food_t *food = dyn_arr_get(food_list, i);
    food_free(food);
  }
  dyn_arr_free(food_list);
  dyn_arr_free(input_list);
  dyn_arr_free(output_list);
  if (ant_network) {
    free_neural_network(ant_network);
  }

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
  static double simulated_time = 0.0;
  static bool first = false;
  static double last_time = 0.0;
  static double simulation_time = 0.0;
  static double last_update_time = 0.0;

  if (warp) {
    if (tick_speed <= 1.1) {
      tick_speed = WARP_SPEED;
    } else {
      // How much faster or slower than 30 FPS the simulation is running
      frame_time_avg = 0.7 * frame_time_avg + 0.3 * GetFrameTime();
      const double ratio = 1 / (30.0 * frame_time_avg);

      if (ratio < 1.1) {
        tick_speed *= 0.95;
      } else if (ratio > 1.4) {
        tick_speed += 1.5;
      }
    }
  } else {
    tick_speed = 1.0;
  }

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
  if (training && random_session) {
    for (int i = 0; i < ant_list.length; i++) {
      ant_t *ant = dyn_arr_get(ant_list, i);
      ant->rotation = (rand() % 360) * DEG2RAD_D;
      ant->pos.x = rand() % WORLD_W;
      ant->pos.y = rand() % WORLD_H;
      ant->has_food = rand() % 4 == 0;
      ant->is_coliding = false;
    }
  }
  train_ants(fixed_delta);
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

  DrawCircleV(v2d_to_v2(spawn), ANT_SPAWN_RADIUS, DARKBLUE);

  for (int i = 0; i < food_list.length; i++) {
    food_t *food = dyn_arr_get(food_list, i);
    food_draw(food);
  }
  for (int i = 0; i < ant_list.length; i++) {
    ant_t *ant = dyn_arr_get(ant_list, i);
    ant_draw(ant);
  }

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
  if (!PER_ANT_NETWORK) {
    ant_network = create_ant_net();
  }
  dyn_arr_init(ant_list);
  dyn_arr_init(food_list);
  dyn_arr_init(input_list);
  dyn_arr_init(output_list);

  // Initialize raylib
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_HIGHDPI);
  InitWindow(0, 0, "Ant Matrix");
  SetTargetFPS(TARGET_FPS);

  ant_texture = LoadTexture("assets/ant.png");

  ant_data = malloc(starting_ants * sizeof(ant_t));
  ant_t *ant_data_ptr = ant_data;
  if (!ant_data) {
    fprintf(stderr, "Failed to allocate memory for ants\n");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < starting_ants; i++) {
    ant_t *ant = ant_data_ptr++;
    if (PER_ANT_NETWORK) {
      ant->net = create_ant_net();
    } else {
      ant->net = ant_network;
    }

    ant->texture = &ant_texture;
    ant->nearest_food = NULL;
    ant->spawn = spawn;
    ant->pos = spawn;
    ant->rotation = (rand() % 360) * DEG2RAD_D;
    ant->has_food = false;
    ant->is_coliding = false;
    dyn_arr_push(ant_list, ant);
  }

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
  for (int i = 0; i < ant_list.length; i++) {
    ant_t *ant = dyn_arr_get(ant_list, i);
    ant_update_nearest_food(ant);

    // Update the neural network
    double inputs[ANN_INPUTS] = {0};

    const vector2d_t spawn_vector = v2d_subtract(ant->spawn, ant->pos);
    const double spawn_vector_length = v2d_length(spawn_vector);
    inputs[0] = cos(ant->rotation);
    inputs[1] = sin(ant->rotation);
    inputs[2] = circle_collide_point((circled_t){ant->spawn, ANT_SPAWN_RADIUS}, ant->pos) ? 1.0 : 0.0;
    inputs[3] = spawn_vector_length > 1e-9 ? spawn_vector.x / spawn_vector_length : -cos(ant->rotation);
    inputs[4] = spawn_vector_length > 1e-9 ? spawn_vector.y / spawn_vector_length : -sin(ant->rotation);

    if (ant->nearest_food) {
      const vector2d_t food_vector = v2d_subtract(ant->nearest_food->pos, ant->pos);
      const double food_vector_length = v2d_length(food_vector);
      inputs[5] =
          circle_collide_point((circled_t){ant->nearest_food->pos, ant->nearest_food->radius}, ant->pos) ? 1.0 : 0.0;
      inputs[6] = food_vector_length > 1e-9 ? food_vector.x / food_vector_length : -cos(ant->rotation);
      inputs[7] = food_vector_length > 1e-9 ? food_vector.y / food_vector_length : -sin(ant->rotation);
    } else {
      inputs[5] = 0.0;
      inputs[6] = 0.0;
      inputs[7] = 0.0;
    }

    inputs[8] = ant->nearest_food ? 1.0 : 0.0;
    inputs[9] = ant->has_food ? 1.0 : 0.0;

    if (training) {
      ant_logic_t logic = ant_train_update(ant, fixed_delta);

      double outputs[ANN_OUTPUTS] = {0};
      outputs[0] = cos(logic.angle);
      outputs[1] = sin(logic.angle);
      // 0.9 to prevent picking step action when other actions should be taken
      outputs[2] = logic.action == ANT_STEP_ACTION ? 0.9 : -1.0;
      outputs[3] = logic.action == ANT_GATHER_ACTION ? 1.0 : -1.0;
      outputs[4] = logic.action == ANT_DROP_ACTION ? 1.0 : -1.0;

      int iterations = 1;

      if (logic.action == ANT_DROP_ACTION) {
        iterations = 40;
      } else if (logic.action == ANT_GATHER_ACTION) {
        iterations = 20;
      }

      // Train the neural network
      for (int j = 0; j < iterations; j++) {
        network_train_step(ant, inputs, outputs);
      }
    } else {
      const double *pred = run_neural_network(ant->net, inputs);
      ant_logic_t logic = {0};
      const double len = hypot(pred[0], pred[1]);
      if (len > 0.0) {
        logic.angle = constrain_angle(atan2(pred[1] / len, pred[0] / len));
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

      ant_run_update(ant, logic, fixed_delta);
      if (rand() % 2000 == 0) {
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
  for (int i = 0; i < food_list.length; i++) {
    food_t *food = dyn_arr_get(food_list, i);
    food_free(food);
  }
  dyn_arr_clear(food_list);

  random_session = rand() % 3 != 0;

  // Position ants at spawn
  for (int i = 0; i < ant_list.length; i++) {
    ant_t *ant = dyn_arr_get(ant_list, i);
    ant->pos = spawn;
    ant->rotation = (rand() % 360) * DEG2RAD_D;
    ant->has_food = false;
    ant->is_coliding = false;
  }

  // Create food away from ants
  if (!random_session || rand() % 2 == 0) {
    for (int i = 0; i < starting_food; i++) {
      vector2d_t food_pos = spawn;
      while (v2d_distance(spawn, food_pos) < min_food_distance) {
        food_pos.x = (rand() % WORLD_H) + spawn.x / 2;
        food_pos.y = rand() % WORLD_H;
      }
      food_t *food =
          create_food(food_pos, food_radius, food_detection_radius,
                      rand() % (max_starting_food_amount - min_starting_food_amount) + min_starting_food_amount);
      dyn_arr_push(food_list, food);
    }
  }
}

static neural_network_t *create_ant_net() {
  const size_t neuron_counts[] = ANN_NEURON_COUNTS;
  neural_network_t *network = create_neural_network((sizeof(neuron_counts) / sizeof(size_t)) - 2, neuron_counts);
  if (!network) {
    fprintf(stderr, "Failed to create neural network\n");
    exit(EXIT_FAILURE);
  }
  // Randomize weights and biases
  const double std = sqrt(6) / sqrt(neuron_counts[0] + neuron_counts[network->num_hidden_layers + 1]);
  randomize_weights(network, -std, std);
  randomize_bias(network, -0.01, 0.01);

  return network;
}

static void network_train_step(ant_t *ant, const double *inputs, const double *outputs) {
  bool run = true;
  const double *input_ptr = inputs;
  const double *output_ptr = outputs;
  size_t m = 1;

  // Accumulate inputs and outputs for batch training with single network
  if (!PER_ANT_NETWORK) {
    dyn_arr_pusharr(input_list, inputs, ANN_INPUTS);
    dyn_arr_pusharr(output_list, outputs, ANN_OUTPUTS);
    run = input_list.length >= ANN_BATCH_SIZE * ANN_INPUTS;
    input_ptr = input_list.data;
    output_ptr = output_list.data;
    m = (size_t)input_list.length / ANN_INPUTS;
  }

  if (run) {
    const double error = train_neural_network(ant->net, m, input_ptr, output_ptr, learning_rate);
    if (epoch % (MAX(1, ((int)2e6 / m))) == 0) {
      printf("Epoch: %d, Error: % .6f, Learning Rate: % .6Lf\n", epoch, error, learning_rate);
    }
    dyn_arr_clear(input_list);
    dyn_arr_clear(output_list);
    learning_rate = fmaxl(learning_rate * (powl(LEARN_RATE_DECAY, (long double)m)), 1e-4);
    epoch++;
  }

  if (!warp && rand() % 10000 == 0) {
    const double *pred;
    pred = run_neural_network(ant->net, inputs);
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
}