#include "util/util.h"

int nearest_16_by_9_height(int width) {
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

// angle - TAU * floor(angle * 1/TAU);
double constrain_angle(double angle) { return angle - TAU * floor(angle * 0.15915494309189535); }

Vector2 v2d_to_v2(vector2d_t v) { return (Vector2){(float)v.x, (float)v.y}; }
vector2d_t v2_to_v2d(Vector2 v) { return (vector2d_t){(double)v.x, (double)v.y}; }
vector2d_t v2d_add(vector2d_t a, vector2d_t b) { return (vector2d_t){a.x + b.x, a.y + b.y}; }
vector2d_t v2d_subtract(vector2d_t a, vector2d_t b) { return (vector2d_t){a.x - b.x, a.y - b.y}; }
double v2d_length(vector2d_t v) { return sqrt(v.x * v.x + v.y * v.y); }
double v2d_length_sqr(vector2d_t v) { return v.x * v.x + v.y * v.y; }
double v2d_distance(vector2d_t a, vector2d_t b) { return v2d_length(v2d_subtract(b, a)); }
double v2d_distance_sqr(vector2d_t a, vector2d_t b) { return v2d_length_sqr(v2d_subtract(b, a)); }
vector2d_t v2d_normalize(vector2d_t v) {
  const double len = v2d_length(v);
  if (len < 1e-9) {
    return (vector2d_t){0.0, 0.0};
  }
  return (vector2d_t){v.x / len, v.y / len};
}

bool circle_collide_point(circled_t c, vector2d_t p) { return v2d_distance_sqr(p, c.center) <= c.radius * c.radius; }

bool circle_collide_circle(circled_t a, circled_t b) {
  return v2d_distance_sqr(a.center, b.center) <= (a.radius + b.radius) * (a.radius * b.radius);
}