#pragma once
#include <stdbool.h>

#include "entities/ant.h"
#include "entities/food.h"
#include "util/util.h"

#define LEARN_RATE 0.01
// 2 positions (spawn/food), 1 has_food, 1 near food, 1 is_coliding
#define ANN_INPUTS 9

// 3 actions, 1 angle (sin/cos)
#define ANN_OUTPUTS 5

#define TARGET_FPS 0
#define TICK_RATE 30
#define SCREEN_W 1920
#define SCREEN_H 1080
#define WORLD_SCALE 1.0f
#define WORLD_W ((int)(SCREEN_W * WORLD_SCALE))
#define WORLD_H ((int)(SCREEN_H * WORLD_SCALE))

#define CAM_SPEED 1000

#define MAX_DELTA 5.0
#define WARP_SPEED 65536.0

extern vec_ant_t ant_vec;
extern vec_food_t food_vec;

void fixed_update(double fixed_delta);
void render(void);
void render_present(void);
void resize_window(int w, int h);
void update(void);
void initialize(void);
void input(void);

inline double enc(double v) { return (v + 1.0) * 0.5; }
inline double dec(double v) { return v * 2.0 - 1.0; }