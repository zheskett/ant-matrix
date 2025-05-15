#pragma once
#include "raylib.h"
#include "util/definitions.h"

#define ANT_SCALE 0.3f
#define ANT_DETECTOR_RADIUS 50.0f
#define ANT_DETECTOR_OFFSET 70.0f
#define ANT_SPEED 100.0f

typedef enum {
  ANT_IDLE,
  ANT_WALKING,
} ant_state_t;

typedef struct {
  Texture2D* texture;
  Vector2 pos;
  float rotation;
  ant_state_t state;
} ant_t;

/**
 * @brief Create a new ant entity.
 *
 * @param x The x position of the ant.
 * @param y The y position of the ant.
 * @param rotation The rotation of the ant in degrees.
 * @return A pointer to the newly created ant entity, or NULL on failure.
 */
ant_t* create_ant(float x, float y, Texture2D* texture, float rotation);

/**
 * @brief Draw the ant entity.
 *
 * @param ant The ant entity to draw.
 */
void draw_ant(ant_t* ant);

/**
 * @brief Update the ant entity.
 *
 * @param ant The ant entity to update.
 * @param delta_time The time since the last update.
 */
void update_ant(ant_t* ant, float delta_time);

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