#pragma once
#include <stdbool.h>

#include "entities/ant.h"
#include "entities/food.h"
#include "util/definitions.h"

#define TARGET_FPS 0
#define TICK_RATE 60
#define SCREEN_W 1920
#define SCREEN_H 1080
#define WORLD_SCALE 4.0f
#define WORLD_W ((int)(SCREEN_W * WORLD_SCALE))
#define WORLD_H ((int)(SCREEN_H * WORLD_SCALE))

#define CAM_SPEED 1000

#define MAX_DELTA 0.25

extern vec_ant_t ant_vec;
extern vec_food_t food_vec;

void fixed_update(double fixed_delta);
void render(void);
void render_present(void);
void resize_window(int w, int h);
void update(void);
void initialize(void);
void input(void);