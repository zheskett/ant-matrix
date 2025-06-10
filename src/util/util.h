/**
 * @file util.h
 * @author Zachary Heskett (zheskett@gmail.com)
 * @brief Utility functions and data structures for the ant simulation program.
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef UTIL_H
#define UTIL_H

#include "raylib.h"

#define TAU 6.28318530717958647693

#define DEG2RAD_D 0.01745329251994329577
#define RAD2DEG_D 57.2957795130823208768

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) <= (b) ? (a) : (b))

/**
 * @brief A vector with 2 components and double precision.
 */
typedef struct {
  double x;
  double y;
} vector2d_t;

/**
 * @brief A circle defined by a center point and a radius.
 *
 * This structure is used to represent circles in 2D space.
 */
typedef struct {
  vector2d_t center;
  double radius;
} circled_t;

/**
 * @brief Get the nearest height for a given width that fits a common 16:9 aspect ratio (scaled down by 1 step).
 *
 * @param width The width to calculate the height for.
 * @return The nearest height that maintains a 16:9 aspect ratio.
 */
int nearest_16_by_9_height(int width);

/**
 * @brief Constrain an angle to the range [-π, π).
 *
 * @param angle The angle in radians to constrain.
 * @return The constrained angle in radians.
 */
double constrain_angle(double angle);

/** @brief Convert a Vector2 to a vector2d_t.
 *
 * @param v The Vector2 to convert.
 * @return The converted vector2d_t.
 */
Vector2 v2d_to_v2(vector2d_t v);

/** @brief Convert a vector2d_t to a Vector2.
 *
 * @param v The vector2d_t to convert.
 * @return The converted Vector2.
 */
vector2d_t v2_to_v2d(Vector2 v);

/**
 * @brief Add two vector2d_t structures together.
 *
 * @param a First vector2d_t to add.
 * @param b Second vector2d_t to add.
 * @return The resulting vector2d_t after addition.
 */
vector2d_t v2d_add(vector2d_t a, vector2d_t b);

/**
 * @brief Subtract one vector2d_t from another.
 *
 * @param a vector2d_t to subtract from.
 * @param b vector2d_t to subtract.
 * @return The resulting vector2d_t after subtraction.
 */
vector2d_t v2d_subtract(vector2d_t a, vector2d_t b);

/**
 * @brief Get the length of a vector2d_t.
 *
 * @param v vector2d_t to calculate the length of.
 * @return The length of the vector2d_t.
 */
double v2d_length(vector2d_t v);

/**
 * @brief Get the squared length of a vector2d_t.
 *
 * @param v vector2d_t to calculate the squared length of.
 * @return The squared length of the vector2d_t.
 */
double v2d_length_sqr(vector2d_t v);

/**
 * @brief Get the distance between two vector2d_t structures.
 *
 * @param a First vector2d_t.
 * @param b Second vector2d_t.
 * @return The distance between the two vector2d_t structures.
 */
double v2d_distance(vector2d_t a, vector2d_t b);

/**
 * @brief Get the squared distance between two vector2d_t structures.
 *
 * @param a First vector2d_t.
 * @param b Second vector2d_t.
 * @return The squared distance between the two vector2d_t structures.
 */
double v2d_distance_sqr(vector2d_t a, vector2d_t b);

/**
 * @brief Normalize a vector2d_t to a unit vector.
 *
 * @param v The vector2d_t to normalize.
 * @return The normalized vector2d_t.
 */
vector2d_t v2d_normalize(vector2d_t v);

/**
 * @brief Check if a circle and point collide.
 *
 * @param c Circle to check for collision.
 * @param p Point to check for collision.
 * @return true if the point is inside the circle, false otherwise.
 * @note (x2 - x1)^2 + (y2 - y1)^2 <= r^2
 */
bool circle_collide_point(circled_t c, vector2d_t p);

/**
 * @brief Check if two circles collide.
 *
 * @param a First circle to check for collision.
 * @param b Second circle to check for collision.
 * @return true if the circles collide, false otherwise.
 * @note (x2 - x1)^2 + (y2 - y1)^2 <= (r1 + r2)^2
 */
bool circle_collide_circle(circled_t a, circled_t b);

#endif /* UTIL_H */