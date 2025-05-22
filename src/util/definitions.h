#pragma once
#include <math.h>

#include "raylib.h"

#define TAU 6.28318530717958647693f

typedef struct {
  Vector2 center;
  float radius;
} Circle;

/**
 * @brief Constrain an angle to the range [0, 2*PI).
 *
 * @param angle The angle to constrain.
 * @return The constrained angle.
 */
inline float constrain_angle(float angle) { return angle - TAU * floorf(angle * 0.15915494309189535f); }