/**
 * @file food.h
 * @author Zachary Heskett (zheskett@gmail.com)
 * @brief Header file for the food entity in the ant simulation program.
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef FOOD_H
#define FOOD_H
#include "util/dynarr.h"
#include "util/util.h"

/**
 * @brief Represents a food object in the simulation.
 */
typedef struct {
  vector2d_t pos;          /**< Position of the food */
  double radius;           /**< Radius of the food object */
  double detection_radius; /**< Radius for detecting food by ants */
  int amount;              /**< Amount of food available */
} food_t;

typedef dyn_arr_def(food_t *) dyn_arr_food_t;

/**
 * @brief Create a food object
 *
 * @param pos Position of the food
 * @param radius Radius of the food
 * @param detection_radius Detection radius of the food
 * @param amount Amount of food
 * @return food_t* Pointer to the created food object
 */
food_t *create_food(vector2d_t pos, double radius, double detection_radius, int amount);

/**
 * @brief Update the food object
 *
 * @param food Pointer to the food object
 * @param delta_time Delta time
 */
void food_update(food_t *food, double delta_time);

/**
 * @brief Draw the food object
 *
 * @param food Pointer to the food object
 */
void food_draw(food_t *food);

/**
 * @brief Destroy the food object
 *
 * @param food Pointer to the food object
 */
void food_free(food_t *food);

/**
 * @brief Take some food from food pile
 *
 * @param food Pointer to the food to take from
 */
void food_grab(food_t *food);

#endif /* FOOD_H */