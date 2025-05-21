#include "ant.h"

#include <math.h>
#include <stdlib.h>

#include "main.h"
#include "raylib.h"
#include "raymath.h"
#include "util/definitions.h"

static ant_logic_t ant_decision(ant_t* ant, float delta_time);

ant_t* create_ant(Vector2 pos, Texture2D* texture, float rotation) {
  ant_t* ant = (ant_t*)malloc(sizeof(ant_t));
  if (!ant) {
    return NULL;
  }

  ant->texture = texture;
  ant->nearest_food = NULL;
  ant->pos = pos;
  ant->spawn = pos;
  ant->rotation = rotation;
  ant->has_food = false;
  ant->is_coliding = false;

  return ant;
}

void update_ant(ant_t* ant, float delta_time) {
  if (!ant) {
    return;
  }

  ant->nearest_food = NULL;

  Circle detector_circle = get_ant_detector_circle(ant);
  float nearest_distance = MAXFLOAT;
  for (int i = 0; i < food_vec.length; i++) {
    food_t* food = food_vec.data[i];
    if (CheckCollisionCircles(detector_circle.center, detector_circle.radius, food->pos, food->detection_radius)) {
      const float distance = Vector2DistanceSqr(ant->pos, food->pos);
      if (distance < nearest_distance) {
        nearest_distance = distance;
        ant->nearest_food = food;
      }
    }
  }

  ant_logic_t logic = ant_decision(ant, delta_time);

  ant_set_angle(ant, logic.angle);
  switch (logic.action) {
    case ANT_STEP_ACTION:
      ant_step(ant, delta_time);
      break;
    case ANT_GATHER_ACTION:
      ant_gather(ant);
      break;
    case ANT_DROP_ACTION:
      ant_drop(ant);
      break;
  }
}

void draw_ant(ant_t* ant) {
  if (!ant) {
    return;
  }

  const Vector2 center = {(ant->texture->width * ANT_SCALE) / 2.0f, (ant->texture->height * ANT_SCALE) / 2.0f};
  const Rectangle source = {0, 0, ant->texture->width, ant->texture->height};
  const Rectangle dest = {ant->pos.x, ant->pos.y, (ant->texture->width * ANT_SCALE),
                          (ant->texture->height * ANT_SCALE)};
  DrawTexturePro(*ant->texture, source, dest, center, ant->rotation * RAD2DEG, WHITE);

  const Circle detector_circle = get_ant_detector_circle(ant);
  DrawCircleV(detector_circle.center, detector_circle.radius,
              ant->has_food ? (Color){0, 255, 0, 128} : (Color){255, 0, 0, 128});
}

void destroy_ant(ant_t* ant) {
  if (ant) {
    free(ant);
  }
}

Circle get_ant_detector_circle(ant_t* ant) {
  if (!ant) {
    return (Circle){(Vector2){0.0f, 0.0f}, 0.0f};
  }

  // Circle center position, accounting for rotation, position at head of the ant
  Vector2 circle_pos = {
      ant->pos.x + (ANT_DETECTOR_OFFSET * ANT_SCALE) * cosf(ant->rotation),
      ant->pos.y + (ANT_DETECTOR_OFFSET * ANT_SCALE) * sinf(ant->rotation),
  };
  const Circle circle = {circle_pos, ANT_DETECTOR_RADIUS * ANT_SCALE};
  return circle;
}

void ant_set_angle(ant_t* ant, float angle) {
  if (!ant) {
    return;
  }

  ant->rotation = constrain_angle(angle);
}

bool ant_step(ant_t* ant, float delta_time) {
  if (!ant) {
    return false;
  }

  ant->pos.x += ANT_SPEED * cosf(ant->rotation) * delta_time;
  ant->pos.y += ANT_SPEED * sinf(ant->rotation) * delta_time;

  // Check for boundary collisions
  if (ant->pos.x < 0 || ant->pos.x > WORLD_W || ant->pos.y < 0 || ant->pos.y > WORLD_H) {
    ant->pos.x = fmaxf(0, fminf(WORLD_W, ant->pos.x));
    ant->pos.y = fmaxf(0, fminf(WORLD_H, ant->pos.y));

    ant->is_coliding = true;
    return false;
  }

  ant->is_coliding = false;
  return true;
}

bool ant_gather(ant_t* ant) {
  if (!ant) {
    return false;
  }

  if (!ant->nearest_food || ant->has_food) {
    return false;
  }

  // Check if the ant has reached the food
  if (CheckCollisionPointCircle(ant->pos, ant->nearest_food->pos, ant->nearest_food->radius)) {
    grab_food(ant->nearest_food);
    ant->has_food = true;
    if (ant->nearest_food->amount <= 0) {
      vec_remove(&food_vec, ant->nearest_food);
      destroy_food(ant->nearest_food);
    }

    return true;
  }

  return false;
}

bool ant_drop(ant_t* ant) {
  if (!ant) {
    return false;
  }

  // Check if the ant has reached the spawn point
  if (CheckCollisionPointCircle(ant->pos, ant->spawn, ANT_SPAWN_RADIUS)) {
    ant->has_food = false;
    return true;
  }

  return false;
}

/**
 * @brief Update the ant's logic and behavior.
 *
 * @param ant Pointer to the ant entity.
 * @param delta_time Time since the last update.
 */
static ant_logic_t ant_decision(ant_t* ant, float delta_time) {
  if (!ant) {
    return (ant_logic_t){ANT_STEP_ACTION, 0.0f};
  }

  float face_direction = 0.0f;
  ant_action_t action = ANT_STEP_ACTION;

  // Collect food
  if (ant->has_food) {
    // Move towards the spawn point
    Vector2 direction = Vector2Normalize(Vector2Subtract(ant->spawn, ant->pos));
    face_direction = constrain_angle(atan2f(direction.y, direction.x));

    if (CheckCollisionPointCircle(ant->pos, ant->spawn, ANT_SPAWN_RADIUS)) {
      action = ANT_DROP_ACTION;
      face_direction = (rand() % 360) * DEG2RAD;
    }
  }

  // Gather food
  else if (ant->nearest_food) {
    // Move towards the closest food
    Vector2 direction = Vector2Normalize(Vector2Subtract(ant->nearest_food->pos, ant->pos));
    face_direction = constrain_angle(atan2f(direction.y, direction.x));

    if (CheckCollisionPointCircle(ant->pos, ant->nearest_food->pos, ant->nearest_food->radius)) {
      action = ANT_GATHER_ACTION;
    }
  }

  else {
    if (ant->is_coliding) {
      face_direction = constrain_angle(ant->rotation + PI);
    }

    // Random chance to change direction
    else if (rand() % 100 < 5) {
      face_direction = constrain_angle(ant->rotation + (float)(rand() % 90 - 45) * DEG2RAD);
    } else {
      face_direction = ant->rotation;
    }
  }

  return (ant_logic_t){action, face_direction};
}