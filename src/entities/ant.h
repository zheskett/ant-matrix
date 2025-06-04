#pragma once
#ifndef ANT_H
#define ANT_H

#include "entities/food.h"
#include "neural/nn.h"
#include "util/util.h"
#include "vec.h"

#define ANT_SCALE 0.25
#define ANT_DETECTOR_RADIUS 50.0
#define ANT_DETECTOR_OFFSET 70.0
#define ANT_SPEED 100.0
#define ANT_SPAWN_RADIUS 50.0

typedef enum {
  ANT_STEP_ACTION,
  ANT_GATHER_ACTION,
  ANT_DROP_ACTION,
} ant_action_t;

typedef struct {
  ant_action_t action;
  double angle;
} ant_logic_t;

typedef struct {
  Texture2D* texture;
  food_t* nearest_food;
  neural_network_t* net;
  vector2d_t spawn;
  vector2d_t pos;
  double rotation;
  bool has_food;
  bool is_coliding;
} ant_t;

typedef vec_t(ant_t*) vec_ant_t;

/**
 * @brief Create a new ant entity.
 *
 * @param pos The initial position of the ant.
 * @param texture The texture of the ant.
 * @param rotation The rotation of the ant in radians.
 * @return A pointer to the newly created ant entity, or NULL on failure.
 */
ant_t* ant_create(vector2d_t pos, vector2d_t spawn, Texture2D* texture, double rotation);

/**
 * @brief Draw the ant entity.
 *
 * @param ant The ant entity to draw.
 */
void ant_draw(ant_t* ant);

/**
 * @brief Update the ant's nearest food.
 *
 * @param ant The ant entity to update.
 */
void ant_update_nearest_food(ant_t* ant);

/**
 * @brief Update the ant entity.
 *
 * @param ant The ant entity to update.
 * @param delta_time The time since the last update.
 */
ant_logic_t ant_train_update(ant_t* ant, double delta_time);

/**
 * @brief Run the ant's update logic.
 *
 * @param ant The ant entity to update.
 * @param logic The ant's logic to run.
 * @param delta_time The time since the last update.
 */
void ant_run_update(ant_t* ant, ant_logic_t logic, double delta_time);

/**
 * @brief Destroy an ant entity and free its resources.
 *
 * @param ant The ant entity to destroy.
 */
void ant_free(ant_t* ant);

/**
 * @brief Get the circle representing the ant's detector.
 *
 * @param ant The ant entity.
 * @return The circle representing the ant's detector.
 */
circled_t ant_get_detector_circle(ant_t* ant);

// Behavior functions

/**
 * @brief Move the ant towards the specified angle.
 *
 * @param ant The ant entity to move.
 * @param angle The angle to move towards in radians.
 */
void ant_set_angle(ant_t* ant, double angle);

/**
 * @brief Move the ant in the faced direction.
 *
 * @param ant The ant entity to move.
 * @param pos The position to move towards.
 *
 * @return true if the ant moved, false otherwise.
 */
bool ant_step(ant_t* ant, double delta_time);

/**
 * @brief Gather food.
 *
 * @param ant The ant entity to gather food.
 *
 * @return true if the ant gathered food, false otherwise.
 */
bool ant_gather(ant_t* ant);

/**
 * @brief Drop food off.
 *
 * @param ant The ant entity to drop food.
 *
 * @return true if the ant dropped food, false otherwise.
 */
bool ant_drop(ant_t* ant);

#endif /* ANT_H */