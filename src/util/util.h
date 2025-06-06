#pragma once
#ifndef UTIL_H
#define UTIL_H

#include "raylib.h"

#define TAU 6.28318530717958647693

#define DEG2RAD_D 0.01745329251994329577
#define RAD2DEG_D 57.2957795130823208768

#define MAX(a, b)                                                                                                      \
  ({                                                                                                                   \
    typeof(a) _a = (a);                                                                                                \
    typeof(b) _b = (b);                                                                                                \
    _a > _b ? _a : _b;                                                                                                 \
  })
#define MIN(a, b)                                                                                                      \
  ({                                                                                                                   \
    typeof(a) _a = (a);                                                                                                \
    typeof(b) _b = (b);                                                                                                \
    _a <= _b ? _a : _b;                                                                                                \
  })
#define enc(v) (((v) + 1.0) * 0.5)
#define dec(v) ((v) * 2.0 - 1.0)

// Vector2 with double precision
typedef struct {
  double x;
  double y;
} vector2d_t;

typedef struct {
  Vector2 center;
  float radius;
} Circle;

typedef struct {
  vector2d_t center;
  double radius;
} circled_t;

int nearest_16_by_9_height(int width);

double constrain_angle(double angle);

Vector2 v2d_to_v2(vector2d_t v);
vector2d_t v2_to_v2d(Vector2 v);
vector2d_t v2d_add(vector2d_t a, vector2d_t b);
vector2d_t v2d_subtract(vector2d_t a, vector2d_t b);
double v2d_length(vector2d_t v);
double v2d_length_sqr(vector2d_t v);
double v2d_distance(vector2d_t a, vector2d_t b);
double v2d_distance_sqr(vector2d_t a, vector2d_t b);
vector2d_t v2d_normalize(vector2d_t v);

// (x2 - x1)^2 + (y2 - y1)^2 <= r^2
bool circle_collide_point(circled_t c, vector2d_t p);

// (x2 - x1)^2 + (y2 - y1)^2 <= (r1 + r2)^2
bool circle_collide_circle(circled_t a, circled_t b);

#endif /* UTIL_H */