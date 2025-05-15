#pragma once
#include "entities/ant.h"
#include "vec.h"

#define TARGET_FPS 0
#define TICK_RATE 60
#define SCREEN_W 1920
#define SCREEN_H 1080
#define MAX_DELTA 0.25

typedef vec_t(ant_t*) vec_ant_t;

void fixed_update(double fixed_delta);
void render(void);
void render_present(void);
void resize_window(int w, int h);
void update(void);