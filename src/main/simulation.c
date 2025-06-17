#include "main/simulation.h"

#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

#include "raymath.h"
#include "util/gui.h"

#pragma region function_declarations

static void input(void);
static void update(void);
static void fixed_update(double fixed_delta);
static void render(void);
static void initialize(void);
static void resize_window(int w, int h);
static void render_present(void);

static void *training_thread_func(void *arg);
static void reset_simulation(void);
static void train_ants(double fixed_delta);
static neural_network_t *create_ant_net();
static void network_train_step(ant_t *ant, const double *inputs, const double *outputs);

#pragma endregion

typedef enum { SINGLE_THREAD, MULTI_THREAD, ENDING_THREAD } simulation_mode_t;

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
 * 0: turn right
 * 1: no turn
 * 2: turn left
 * 3: step
 * 4: gather
 * 5: drop
 */
neural_network_t *ant_network = NULL;
double learning_rate = LEARN_RATE;
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
dyn_arr_ant_t g_ant_list;
dyn_arr_food_t g_food_list;

int epoch = 0;
const int starting_ants = 100;
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

pthread_t training_thread = {0};
atomic_bool run_training_thread = false;
atomic_bool training_thread_done = false;
simulation_mode_t simulation_mode = SINGLE_THREAD;

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

  if (simulation_mode == MULTI_THREAD || simulation_mode == ENDING_THREAD) {
    atomic_store(&run_training_thread, false);
    pthread_join(training_thread, NULL);
  }

  UnloadRenderTexture(offscreen);
  UnloadTexture(ant_texture);

  if (ant_data) {
    for (int i = 0; i < g_ant_list.length; i++) {
      ant_t *ant = dyn_arr_get(g_ant_list, i);
      if (PER_ANT_NETWORK) {
        neural_free(ant->net);
      }
    }
    free(ant_data);
  }
  dyn_arr_free(g_ant_list);

  for (int i = 0; i < g_food_list.length; i++) {
    food_t *food = dyn_arr_get(g_food_list, i);
    food_free(food);
  }
  dyn_arr_free(g_food_list);
  dyn_arr_free(input_list);
  dyn_arr_free(output_list);
  if (ant_network) {
    neural_free(ant_network);
  }

  CloseWindow();

  return 0;
}

static void input() {
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

static void update() {
  const double scaled_delta = FIXED_DELTA / tick_speed;
  static double simulated_time = 0.0;
  static bool first = false;
  static double last_time = 0.0;
  static double simulation_time = 0.0;

  if (reset) {
    reset = false;
    if (simulation_mode == SINGLE_THREAD) {
      first = false;
      simulation_time = 0.0;
      simulated_time = 0.0;

      reset_simulation();
    }
  }

  if (!first) {
    last_time = GetTime();
    first = true;
  }
  const double current_time = GetTime();
  const double delta_time = fmin(current_time - last_time, MAX_DELTA);

  last_time = current_time;
  simulation_time += delta_time;

  if (warp && simulation_mode == SINGLE_THREAD) {
    if (tick_speed <= 1.1) {
      tick_speed = WARP_SPEED;
    } else {
      // How much faster or slower than 15 FPS the simulation is running
      const double alpha = 0.1;
      frame_time_avg = alpha * frame_time_avg + (1 - alpha) * delta_time;
      const double ratio = fmax(0.99, 1 / (15.0 * frame_time_avg));
      if (ratio > 2.5) {
        tick_speed *= 1.0075;
      } else if (ratio > 1.2) {
        tick_speed += 0.1;
      } else if (ratio > 1.0) {
        tick_speed -= 4;
      } else if (ratio < 1.0) {
        tick_speed *= ratio;
      }
    }
  } else {
    tick_speed = 1.0;
  }

  while (simulation_time >= scaled_delta) {
    simulation_time -= scaled_delta;
    simulated_time += FIXED_DELTA;

    if (simulation_mode == SINGLE_THREAD) {
      // Reset simulation every RESET_TIME (scaled by tick speed)
      if (simulated_time >= RESET_TIME && auto_reset) {
        reset_simulation();
        simulated_time = 0.0;
      }

      fixed_update(FIXED_DELTA);
    }
  }

  if (simulation_mode == ENDING_THREAD) {
    if (atomic_load(&training_thread_done)) {
      pthread_join(training_thread, NULL);
      simulation_mode = SINGLE_THREAD;
      atomic_store(&training_thread_done, false);
    }
  }
}

static void fixed_update(double fixed_delta) { train_ants(fixed_delta); }

static void render() {
  Camera2D cam = {0};
  cam.target = target;
  cam.offset = (Vector2){SCREEN_W / 2.0f, SCREEN_H / 2.0f};
  cam.rotation = 0.0f;
  cam.zoom = zoom;

  Vector2 mouse_pos = GetMousePosition();
  mouse_pos.x = Remap(mouse_pos.x - letterbox.x, 0, window_w - 2 * letterbox.x, 0, SCREEN_W);
  mouse_pos.y = Remap(mouse_pos.y - letterbox.y, 0, window_h - 2 * letterbox.y, 0, SCREEN_H);

  BeginTextureMode(offscreen);
  BeginMode2D(cam);
  ClearBackground(BROWN);

  if (simulation_mode == SINGLE_THREAD) {
    DrawCircleV(v2d_to_v2(spawn), ANT_SPAWN_RADIUS, DARKBLUE);

    for (int i = 0; i < g_food_list.length; i++) {
      food_t *food = dyn_arr_get(g_food_list, i);
      food_draw(food);
    }
    for (int i = 0; i < g_ant_list.length; i++) {
      ant_t *ant = dyn_arr_get(g_ant_list, i);
      ant_draw(ant);
    }

    EndMode2D();
  } else {
    EndMode2D();

    gui_draw_label_centered((Vector2){SCREEN_W / 2, SCREEN_W / 2},
                            "Running in multi-thread mode, rendering is disabled.");
  }

  DrawFPS(0, 0);
  if (gui_draw_button(mouse_pos, (Rectangle){10, 20, 300, 20}, "Reset Speed")) {
    warp = false;
  }

  // Button to start runing the simulation from the network
  if (gui_draw_button(mouse_pos, (Rectangle){SCREEN_W - 350, 10, 300, 20}, "Reset Simulation")) {
    reset = true;
  }

  // Checkbox to start runing the simulation from the network
  // This checkbox is always on in multi-thread mode
  const bool train_checkbox = gui_draw_checkbox(mouse_pos, (Vector2){SCREEN_W - 350, 40}, "Train", training);
  if (simulation_mode == SINGLE_THREAD) {
    training = train_checkbox;
  }

  // Checkbox to enable/disable auto reset
  auto_reset = gui_draw_checkbox(mouse_pos, (Vector2){SCREEN_W - 350, 65}, "Auto Reset", auto_reset);

  // Checkbox to enable/disable warp
  warp = gui_draw_checkbox(mouse_pos, (Vector2){SCREEN_W - 350, 90}, "Warp", warp);

  Rectangle thread_button_bounds = {SCREEN_W - 350, 115, 300, 20};
  // Checkbox to do multiple threads
  if (simulation_mode == SINGLE_THREAD) {
    if (gui_draw_button(mouse_pos, thread_button_bounds, "Switch to Multi-Thread")) {
      simulation_mode = MULTI_THREAD;
      atomic_store(&run_training_thread, true);
      atomic_store(&training_thread_done, false);
      training = true;

      const int error = pthread_create(&training_thread, NULL, training_thread_func, NULL);
      if (error != 0) {
        fprintf(stderr, "Failed to create training thread: %s\n", strerror(error));
        exit(EXIT_FAILURE);
      }
    }
  } else if (simulation_mode == MULTI_THREAD) {
    if (gui_draw_button(mouse_pos, thread_button_bounds, "Switch to Single-Thread")) {
      simulation_mode = ENDING_THREAD;
      atomic_store(&run_training_thread, false);
    }
  }

  gui_draw_label((Vector2){10, 45}, TextFormat("Tick Speed: %.0f", tick_speed));

  if (g_ant_list.length > 0) {
    ant_t *ant = dyn_arr_get(g_ant_list, 0);
    gui_draw_neural_network((Vector2){10, 70}, ant->net, !training);
  }

  if (training) {
    const float progress_bar_location_x = SCREEN_W / 2.0f;
    const float progress_bar_width = 300.0f;
    const Rectangle progress_bar_bounds = {progress_bar_location_x - progress_bar_width / 2.0f, SCREEN_W / 2.0f + 40,
                                           progress_bar_width, 20};
    // Learning rate decays logarithmically, so we need to adjust the progress accordingly
    float progress = 1.0f - log(learning_rate / LEARN_RATE_MIN) / log(LEARN_RATE / LEARN_RATE_MIN);
    progress = fmaxf(0.0f, fminf(1.0f, progress));
    gui_draw_progress_bar(progress_bar_bounds, progress, "Training Progress");
  }

  EndTextureMode();

  if (simulation_mode != SINGLE_THREAD) {
    training = true;
    reset = false;
  }
}

static void initialize() {
  srand(time(NULL));
  if (!PER_ANT_NETWORK) {
    ant_network = create_ant_net();
  }
  dyn_arr_init(g_ant_list);
  dyn_arr_init(g_food_list);
  dyn_arr_init(input_list);
  dyn_arr_init(output_list);

  // Initialize raylib
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_HIGHDPI);
  SetTraceLogLevel(LOG_DEBUG);
  InitWindow(1280, 720, "Ant Matrix");
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
    dyn_arr_push(g_ant_list, ant);
  }

  reset_simulation();

  offscreen = LoadRenderTexture(SCREEN_W, SCREEN_H);
  SetTextureFilter(offscreen.texture, TEXTURE_FILTER_BILINEAR);
#ifdef __EMSCRIPTEN__
  resize_window(GetScreenWidth(), GetScreenHeight());
#else
  const int height = nearest_16_by_9_height(GetMonitorWidth(GetCurrentMonitor()));
  resize_window(height * 16 / 9, height);
  // Position the window in the center of the screen
  SetWindowPosition((GetMonitorWidth(GetCurrentMonitor()) - window_w) / 2,
                    (GetMonitorHeight(GetCurrentMonitor()) - window_h) / 2);
#endif
}

static void *training_thread_func(void *arg) {
  (void)arg; // Unused parameter
  double run_time = 0.0;

  while (atomic_load(&run_training_thread)) {
    for (int i = 0; i < THREAD_TICKS_PER_CHECK; i++) {
      train_ants(FIXED_DELTA);
      run_time += FIXED_DELTA;

      if (run_time >= RESET_TIME) {
        run_time = 0.0;
        reset_simulation();
      }
    }
  }

  atomic_store(&training_thread_done, true);
  return NULL;
}

static void resize_window(int w, int h) {
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

static void render_present() {
  /* render offscreen to display */

  BeginDrawing();
  ClearBackground(BROWN);
  DrawRectangle(0, 0, letterbox.x, GetScreenHeight(), WHITE);
  DrawRectangle(letterbox.x + letterbox.width, 0, GetScreenWidth() - (letterbox.x + letterbox.width), GetScreenHeight(),
                WHITE);

  const Rectangle render_src = {0, 0, (float)SCREEN_W, -(float)SCREEN_H};
  const Vector2 render_origin = {0, 0};
  DrawTexturePro(offscreen.texture, render_src, letterbox, render_origin, 0.0f, WHITE);
  EndDrawing();
}

static void train_ants(double fixed_delta) {
  if (training && random_session) {
    for (int i = 0; i < g_ant_list.length; i++) {
      ant_t *ant = dyn_arr_get(g_ant_list, i);
      ant->rotation = (rand() % 360) * DEG2RAD_D;
      ant->pos.x = rand() % WORLD_W;
      ant->pos.y = rand() % WORLD_H;
      ant->has_food = rand() % 4 == 0;
      ant->is_coliding = false;
    }
  }

  for (int i = 0; i < g_ant_list.length; i++) {
    ant_t *ant = dyn_arr_get(g_ant_list, i);
    ant_update_nearest_food(ant);

    // Update the neural network
    double inputs[ANN_INPUTS] = {0};

    const vector2d_t spawn_vector = v2d_subtract(ant->spawn, ant->pos);
    const double spawn_vector_length = v2d_length(spawn_vector);
    inputs[0] = cos(ant->rotation);
    inputs[1] = sin(ant->rotation);
    inputs[2] = circle_collide_point((circled_t){ant->spawn, ANT_SPAWN_RADIUS}, ant->pos) ? 1.0 : 0.0;
    inputs[3] = spawn_vector_length > 1e-9 ? enc(spawn_vector.x / spawn_vector_length) : enc(-cos(ant->rotation));
    inputs[4] = spawn_vector_length > 1e-9 ? enc(spawn_vector.y / spawn_vector_length) : enc(-sin(ant->rotation));

    if (ant->nearest_food) {
      const vector2d_t food_vector = v2d_subtract(ant->nearest_food->pos, ant->pos);
      const double food_vector_length = v2d_length(food_vector);
      inputs[5] =
          circle_collide_point((circled_t){ant->nearest_food->pos, ant->nearest_food->radius}, ant->pos) ? 1.0 : 0.0;
      inputs[6] = food_vector_length > 1e-9 ? enc(food_vector.x / food_vector_length) : enc(-cos(ant->rotation));
      inputs[7] = food_vector_length > 1e-9 ? enc(food_vector.y / food_vector_length) : enc(-sin(ant->rotation));
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
      outputs[0] = logic.turn_action == ANT_TURN_RIGHT ? 1.0 : 0.0;
      outputs[1] = logic.turn_action == ANT_TURN_NONE ? 1.0 : 0.0;
      outputs[2] = logic.turn_action == ANT_TURN_LEFT ? 1.0 : 0.0;

      outputs[3] = logic.action == ANT_STEP_ACTION ? 1.0 : 0.0;
      outputs[4] = logic.action == ANT_GATHER_ACTION ? 1.0 : 0.0;
      outputs[5] = logic.action == ANT_DROP_ACTION ? 1.0 : 0.0;

      int iterations = 1;

      if (logic.action == ANT_DROP_ACTION) {
        iterations = 30;
      } else if (logic.action == ANT_GATHER_ACTION) {
        iterations = 10;
      } else if (inputs[5] >= 1.0) {
        iterations = 5;
      }

      // Train the neural network
      for (int j = 0; j < iterations; j++) {
        network_train_step(ant, inputs, outputs);
      }
    } else {
      vector_t inputs_vec = {inputs, ANN_INPUTS};
      const double *pred = neural_run(ant->net, &inputs_vec)->data;
      ant_logic_t logic = {0};

      int choice = 0;
      logic.turn_action = ANT_TURN_RIGHT;
      if (pred[1] > pred[choice]) {
        logic.turn_action = ANT_TURN_NONE;
        choice = 1;
      }
      if (pred[2] > pred[choice]) {
        logic.turn_action = ANT_TURN_LEFT;
      }

      choice = 3;
      logic.action = ANT_STEP_ACTION;
      if (pred[4] > pred[choice]) {
        logic.action = ANT_GATHER_ACTION;
        choice = 4;
      }
      if (pred[5] > pred[choice]) {
        logic.action = ANT_DROP_ACTION;
      }

      ant_run_update(ant, logic, fixed_delta);
      if (rand() % 5000 == 0) {
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
  for (int i = 0; i < g_food_list.length; i++) {
    food_t *food = dyn_arr_get(g_food_list, i);
    food_free(food);
  }
  dyn_arr_clear(g_food_list);

  random_session = rand() % 3 != 0;

  // Position ants at spawn
  for (int i = 0; i < g_ant_list.length; i++) {
    ant_t *ant = dyn_arr_get(g_ant_list, i);
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
        food_pos.x = (rand() % WORLD_W);
        food_pos.y = rand() % WORLD_H;
      }
      food_t *food =
          food_create(food_pos, food_radius, food_detection_radius,
                      rand() % (max_starting_food_amount - min_starting_food_amount) + min_starting_food_amount);
      dyn_arr_push(g_food_list, food);
    }
  }
}

static neural_network_t *create_ant_net() {
  const int neuron_counts[] = ANN_NEURON_COUNTS;
  neural_network_t *network = neural_create((sizeof(neuron_counts) / sizeof(int)) - 2, neuron_counts);
  if (!network) {
    fprintf(stderr, "Failed to create neural network\n");
    exit(EXIT_FAILURE);
  }
  // Randomize weights and biases
  const double std = sqrt(6) / sqrt(neuron_counts[0] + neuron_counts[network->num_hidden_layers + 1]);
  neural_randomize_weights(network, -std, std);
  neural_randomize_bias(network, -0.01, 0.01);

  return network;
}

static void network_train_step(ant_t *ant, const double *inputs, const double *outputs) {
  bool run = true;
  const double *input_ptr = inputs;
  const double *output_ptr = outputs;
  int m = 1;

  // Accumulate inputs and outputs for batch training with single network
  if (!PER_ANT_NETWORK) {
    dyn_arr_pusharr(input_list, inputs, ANN_INPUTS);
    dyn_arr_pusharr(output_list, outputs, ANN_OUTPUTS);
    run = input_list.length >= ANN_BATCH_SIZE * ANN_INPUTS;
    input_ptr = input_list.data;
    output_ptr = output_list.data;
    m = input_list.length / ANN_INPUTS;
  }

  if (run) {
    const matrix_t input_matrix = {(double *)input_ptr, m, ANN_INPUTS};
    const matrix_t output_matrix = {(double *)output_ptr, m, ANN_OUTPUTS};
    const double error = neural_train(ant->net, &input_matrix, &output_matrix, learning_rate);
    if (epoch % (MAX(1, ((int)2e6 / m))) == 0) {
      printf("Epoch: %d, Error: % .6f, Learning Rate: % .6f\n", epoch, error, learning_rate);
    }
    dyn_arr_clear(input_list);
    dyn_arr_clear(output_list);
    learning_rate = fmax(learning_rate * (pow(LEARN_RATE_DECAY, (double)m)), LEARN_RATE_MIN);
    epoch++;
  }

  if (simulation_mode == SINGLE_THREAD && !warp && rand() % 10000 == 0) {
    const double *pred;
    const vector_t inputs_vec = {(double *)inputs, ANN_INPUTS};
    pred = neural_run(ant->net, &inputs_vec)->data;
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