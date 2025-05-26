#pragma once
#include <stdbool.h>

#include "entities/food.h"
#include "raylib.h"
#include "util/util.h"
#include "vec.h"

#define ANT_SCALE 0.25f
#define ANT_DETECTOR_RADIUS 50.0f
#define ANT_DETECTOR_OFFSET 70.0f
#define ANT_SPEED 100.0f
#define ANT_SPAWN_RADIUS 50.0f

typedef enum {
  ANT_STEP_ACTION,
  ANT_GATHER_ACTION,
  ANT_DROP_ACTION,
} ant_action_t;

typedef struct {
  ant_action_t action;
  float angle;
} ant_logic_t;

typedef struct {
  Texture2D* texture;
  food_t* nearest_food;
  Vector2 spawn;
  Vector2 pos;
  float rotation;
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
ant_t* create_ant(Vector2 pos, Texture2D* texture, float rotation);

/**
 * @brief Draw the ant entity.
 *
 * @param ant The ant entity to draw.
 */
void draw_ant(ant_t* ant);

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
ant_logic_t train_update_ant(ant_t* ant, float delta_time);

/**
 * @brief Run the ant's update logic.
 *
 * @param ant The ant entity to update.
 * @param logic The ant's logic to run.
 * @param delta_time The time since the last update.
 */
void run_update_ant(ant_t* ant, ant_logic_t logic, float delta_time);

/**
 * @brief Destroy an ant entity and free its resources.
 *
 * @param ant The ant entity to destroy.
 */
void destroy_ant(ant_t* ant);

/**
 * @brief Get the circle representing the ant's detector.
 *
 * @param ant The ant entity.
 * @return The circle representing the ant's detector.
 */
Circle get_ant_detector_circle(ant_t* ant);

// Behavior functions

/**
 * @brief Move the ant towards the specified angle.
 *
 * @param ant The ant entity to move.
 * @param angle The angle to move towards in radians.
 */
void ant_set_angle(ant_t* ant, float angle);

/**
 * @brief Move the ant in the faced direction.
 *
 * @param ant The ant entity to move.
 * @param pos The position to move towards.
 *
 * @return true if the ant moved, false otherwise.
 */
bool ant_step(ant_t* ant, float delta_time);

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