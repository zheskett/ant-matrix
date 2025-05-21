#pragma once
#include "raylib.h"

#define TAU 6.28318530717958647693f
#define constrain_angle(angle) (angle - TAU * floorf(angle * (1.0f / TAU)))

typedef struct {
  Vector2 center;
  float radius;
} Circle;