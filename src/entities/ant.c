#include "ant.h"

#include <math.h>
#include <stdlib.h>

#include "main.h"
#include "raylib.h"
#include "raymath.h"
#include "util/definitions.h"
#include "vec.h"

static void ant_logic(ant_t* ant, float delta_time);

ant_t* create_ant(Vector2 pos, Texture2D* texture, float rotation) {
  ant_t* ant = (ant_t*)malloc(sizeof(ant_t));
  if (!ant) {
    return NULL;
  }

  vec_init(&ant->nearby_food);
  vec_reserve(&ant->nearby_food, 8);

  ant->texture = texture;
  ant->pos = pos;
  ant->spawn = pos;
  ant->rotation = rotation;
  ant->has_food = false;
  ant->state = ANT_IDLE;

  return ant;
}

void update_ant(ant_t* ant, float delta_time) {
  if (!ant) {
    return;
  }

  vec_clear(&ant->nearby_food);
  Circle detector_circle = get_ant_detector_circle(ant);
  for (int i = 0; i < food_vec.length; i++) {
    food_t* food = food_vec.data[i];
    if (CheckCollisionCircles(detector_circle.center, detector_circle.radius, food->pos, food->detection_radius)) {
      vec_push(&ant->nearby_food, food);
    }
  }

  ant_logic(ant, delta_time);
}

void draw_ant(ant_t* ant) {
  if (!ant) {
    return;
  }

  const Vector2 center = {(ant->texture->width * ANT_SCALE) / 2.0f, (ant->texture->height * ANT_SCALE) / 2.0f};
  const Rectangle source = {0, 0, ant->texture->width, ant->texture->height};
  const Rectangle dest = {ant->pos.x, ant->pos.y, (ant->texture->width * ANT_SCALE),
                          (ant->texture->height * ANT_SCALE)};
  DrawTexturePro(*ant->texture, source, dest, center, ant->rotation, WHITE);

  const Circle detector_circle = get_ant_detector_circle(ant);
  DrawCircleV(detector_circle.center, detector_circle.radius,
              ant->has_food ? (Color){0, 255, 0, 128} : (Color){255, 0, 0, 128});
}

void destroy_ant(ant_t* ant) {
  if (ant) {
    vec_deinit(&ant->nearby_food);
    free(ant);
  }
}

Circle get_ant_detector_circle(ant_t* ant) {
  if (!ant) {
    return (Circle){(Vector2){0.0f, 0.0f}, 0.0f};
  }

  // Circle center position, accounting for rotation, position at head of the ant
  Vector2 circle_pos = {
      ant->pos.x + (ANT_DETECTOR_OFFSET * ANT_SCALE) * cosf(DEG2RAD * ant->rotation),
      ant->pos.y + (ANT_DETECTOR_OFFSET * ANT_SCALE) * sinf(DEG2RAD * ant->rotation),
  };
  const Circle circle = {circle_pos, ANT_DETECTOR_RADIUS * ANT_SCALE};
  return circle;
}

/**
 * @brief Update the ant's logic and behavior.
 *
 * @param ant Pointer to the ant entity.
 * @param delta_time Time since the last update.
 */
static void ant_logic(ant_t* ant, float delta_time) {
  if (!ant) {
    return;
  }

  // Collect food
  if (ant->state == ANT_RETURNING) {
    if (!ant->has_food) {
      ant->state = ANT_IDLE;
    }

    // Move towards the spawn point
    Vector2 direction = Vector2Normalize(Vector2Subtract(ant->spawn, ant->pos));
    ant->rotation = RAD2DEG * atan2f(direction.y, direction.x);

    // keep rotation within 0-360 degrees
    while (ant->rotation < 0) {
      ant->rotation += 360;
    }
    while (ant->rotation >= 360) {
      ant->rotation -= 360;
    }

    ant->pos = Vector2Add(ant->pos, Vector2Scale(direction, ANT_SPEED * delta_time));

    // Check if the ant has reached the spawn point
    Circle detector_circle = get_ant_detector_circle(ant);
    if (CheckCollisionCircles(detector_circle.center, detector_circle.radius, ant->spawn, ANT_SPAWN_RADIUS)) {
      ant->has_food = false;
      ant->state = ANT_IDLE;
    }
  }

  // Gather food
  if (ant->nearby_food.length > 0 && !ant->has_food) {
    ant->state = ANT_COLLECTING;

    food_t* closest_food = NULL;
    float closest_distance = MAXFLOAT;

    food_t* itr = NULL;
    int i = 0;
    vec_foreach(&ant->nearby_food, itr, i) {
      const float distance = Vector2DistanceSqr(itr->pos, ant->pos);
      if (distance < closest_distance) {
        closest_distance = distance;
        closest_food = itr;
      }
    }

    // Move towards the closest food
    Vector2 direction = Vector2Normalize(Vector2Subtract(closest_food->pos, ant->pos));
    ant->rotation = RAD2DEG * atan2f(direction.y, direction.x);

    // keep rotation within 0-360 degrees
    while (ant->rotation < 0) {
      ant->rotation += 360;
    }
    while (ant->rotation >= 360) {
      ant->rotation -= 360;
    }

    ant->pos = Vector2Add(ant->pos, Vector2Scale(direction, ANT_SPEED * delta_time));

    // Check if the ant has reached the food
    Circle detector_circle = get_ant_detector_circle(ant);
    if (CheckCollisionCircles(detector_circle.center, detector_circle.radius, closest_food->pos,
                              closest_food->radius)) {
      grab_food(closest_food);
      ant->has_food = true;
      if (closest_food->amount <= 0) {
        vec_remove(&food_vec, closest_food);
        vec_remove(&ant->nearby_food, closest_food);
        destroy_food(closest_food);
      }
      ant->state = ANT_RETURNING;
    }

  } else if (ant->state == ANT_COLLECTING) {
    ant->state = ANT_IDLE;
  }

  if (ant->state == ANT_IDLE) {
    // Random chance to change state
    if (rand() % 100 < 4) {
      ant->state = ANT_WALKING;
      ant->rotation = (float)(rand() % 360);
    }
  }

  else if (ant->state == ANT_WALKING) {
    // Random chance to change direction
    if (rand() % 100 < 5) {
      ant->rotation += (float)(rand() % 90 - 45);
    }

    ant->pos.x += ANT_SPEED * cosf(DEG2RAD * ant->rotation) * delta_time;
    ant->pos.y += ANT_SPEED * sinf(DEG2RAD * ant->rotation) * delta_time;

    // Random chance to change state
    if (rand() % 100 < 1) {
      ant->state = ANT_IDLE;
    }
  }

  // Check for boundary collisions
  if (ant->pos.x < 0 || ant->pos.x > WORLD_W || ant->pos.y < 0 || ant->pos.y > WORLD_H) {
    ant->rotation += 180.0f;  // Reverse direction
    if (ant->pos.x < 0) {
      ant->pos.x = 0;
    } else if (ant->pos.x > WORLD_W) {
      ant->pos.x = WORLD_W;
    }
    if (ant->pos.y < 0) {
      ant->pos.y = 0;
    } else if (ant->pos.y > WORLD_H) {
      ant->pos.y = WORLD_H;
    }
  }
}