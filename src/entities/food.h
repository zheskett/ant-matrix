#pragma once
#include "raylib.h"
#include "vec.h"

typedef struct {
  Vector2 pos;
  float radius;
  float detection_radius;
  int amount;
} food_t;

typedef vec_t(food_t*) vec_food_t;

/**
 * @brief Create a food object
 *
 * @param pos Position of the food
 * @param radius Radius of the food
 * @param detection_radius Detection radius of the food
 * @param amount Amount of food
 * @return food_t* Pointer to the created food object
 */
food_t* create_food(Vector2 pos, float radius, float detection_radius, int amount);

/**
 * @brief Update the food object
 *
 * @param food Pointer to the food object
 * @param delta_time Delta time
 */
void update_food(food_t* food, float delta_time);

/**
 * @brief Draw the food object
 *
 * @param food Pointer to the food object
 */
void draw_food(food_t* food);

/**
 * @brief Destroy the food object
 *
 * @param food Pointer to the food object
 */
void destroy_food(food_t* food);

/**
 * @brief Take some food from food pile
 *
 * @param food Pointer to the food to take from
 */
void grab_food(food_t* food);