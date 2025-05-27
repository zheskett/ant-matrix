#pragma once
#include <math.h>

#include "raylib.h"

#define TAU 6.28318530717958647693
#define constrain_angle(angle) ((angle) - TAU * floor((angle) * 0.15915494309189535))
#define enc(v) (((v) + 1.0) * 0.5)
#define dec(v) ((v) * 2.0 - 1.0)

typedef struct {
  Vector2 center;
  float radius;
} Circle;

int nearest_16_by_9_height(int width);