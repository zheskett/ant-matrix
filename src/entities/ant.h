/**
 * @file ant.h
 * @author Zachary Heskett (zheskett@gmail.com)
 * @brief Header file for the ant entity in the ant simulation program.
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef ANT_H
#define ANT_H

#include "entities/food.h"
#include "neural/nn.h"

#define ANT_SCALE 0.25
#define ANT_DETECTOR_RADIUS 50.0
#define ANT_DETECTOR_OFFSET 70.0
#define ANT_SPEED 100.0
#define ANT_SPAWN_RADIUS 50.0

/**
 * @brief Represents the actions an ant can take.
 */
typedef enum {
  ANT_STEP_ACTION,
  ANT_GATHER_ACTION,
  ANT_DROP_ACTION,
} ant_action_t;

/**
 * @brief Represents the output logic for an ant on a given tick.
 */
typedef struct {
  ant_action_t action;
  double angle;
} ant_logic_t;

/**
 * @brief Represents an ant entity in the simulation.
 */
typedef struct {
  Texture2D *texture;    /**< Pointer to the texture of the ant */
  food_t *nearest_food;  /**< The nearest detected food object (in food list) */
  neural_network_t *net; /**< Pointer to the neural network for the ant's behavior */
  vector2d_t spawn;      /**< The spawn position of the ant */
  vector2d_t pos;        /**< The current position of the ant */
  double rotation;       /**< The current rotation of the ant in radians */
  bool has_food;         /**< Whether the ant is currently carrying food */
  bool is_coliding;      /**< Whether the ant is currently colliding with something */
} ant_t;

typedef dyn_arr_def(ant_t *) dyn_arr_ant_t;

/**
 * @brief Create a new ant entity.
 *
 * @param pos The initial position of the ant.
 * @param texture The texture of the ant.
 * @param rotation The rotation of the ant in radians.
 * @return A pointer to the newly created ant entity, or NULL on failure.
 */
ant_t *ant_create(vector2d_t pos, vector2d_t spawn, Texture2D *texture, double rotation);

/**
 * @brief Draw the ant entity.
 *
 * @param ant The ant entity to draw.
 */
void ant_draw(ant_t *ant);

/**
 * @brief Update the ant's nearest food.
 *
 * @param ant The ant entity to update.
 */
void ant_update_nearest_food(ant_t *ant);

/**
 * @brief Update the ant entity.
 *
 * @param ant The ant entity to update.
 * @param delta_time The time since the last update.
 */
ant_logic_t ant_train_update(ant_t *ant, double delta_time);

/**
 * @brief Run the ant's update logic.
 *
 * @param ant The ant entity to update.
 * @param logic The ant's logic to run.
 * @param delta_time The time since the last update.
 */
void ant_run_update(ant_t *ant, ant_logic_t logic, double delta_time);

/**
 * @brief Destroy an ant entity and free its resources.
 *
 * @param ant The ant entity to destroy.
 */
void ant_free(ant_t *ant);

/**
 * @brief Get the circle representing the ant's detector.
 *
 * @param ant The ant entity.
 * @return The circle representing the ant's detector.
 */
circled_t ant_get_detector_circle(ant_t *ant);

// Behavior functions

/**
 * @brief Move the ant towards the specified angle.
 *
 * @param ant The ant entity to move.
 * @param angle The angle to move towards in radians.
 */
void ant_set_angle(ant_t *ant, double angle);

/**
 * @brief Move the ant in the faced direction.
 *
 * @param ant The ant entity to move.
 * @param pos The position to move towards.
 *
 * @return true if the ant moved, false otherwise.
 */
bool ant_step(ant_t *ant, double delta_time);

/**
 * @brief Gather food.
 *
 * @param ant The ant entity to gather food.
 *
 * @return true if the ant gathered food, false otherwise.
 */
bool ant_gather(ant_t *ant);

/**
 * @brief Drop food off.
 *
 * @param ant The ant entity to drop food.
 *
 * @return true if the ant dropped food, false otherwise.
 */
bool ant_drop(ant_t *ant);

#endif /* ANT_H */