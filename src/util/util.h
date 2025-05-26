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

inline const int nearest_16_by_9_height(int width) {
  // Nearest 16:9 standard resolution, 1 res below
  if (width <= 640) {
    return 240;
  } else if (width <= 854) {
    return 360;
  } else if (width <= 1280) {
    return 480;
  } else if (width <= 1920) {
    return 720;
  } else if (width <= 2560) {
    return 1080;
  } else if (width <= 3840) {
    return 1440;
  } else {
    return 2160;
  }
}