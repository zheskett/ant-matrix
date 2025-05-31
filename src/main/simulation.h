#pragma once

#include "entities/ant.h"
#include "entities/food.h"
#include "util/util.h"

#define LEARN_RATE 0.1
#define LEARN_RATE_DECAY 0.999995
// 1 angle, 2 positions (spawn/food), 1 has_food, 1 near food, 1 is_coliding
#define ANN_INPUTS 11

// 3 actions, 1 angle (sin/cos)
#define ANN_OUTPUTS 5

#define ANN_HIDDEN_LAYERS 2
#define ANN_HIDDEN_NODES 16

#define TARGET_FPS 0
#define TICK_RATE 30
#define SCREEN_W 1920
#define SCREEN_H 1080
#define WORLD_SCALE 2.0f
#define WORLD_W ((int)(SCREEN_W * WORLD_SCALE))
#define WORLD_H ((int)(SCREEN_H * WORLD_SCALE))

#define CAM_SPEED 1000

#define MAX_DELTA 5.0
#define WARP_SPEED 100.0
#define RESET_TIME 60.0

extern vec_ant_t ant_vec;
extern vec_food_t food_vec;

int start(int argc, char **argv);
void fixed_update(double fixed_delta);
void render(void);
void render_present(void);
void resize_window(int w, int h);
void update(void);
void initialize(void);
void input(void);