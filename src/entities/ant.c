#include "ant.h"

#include <float.h>
#include <math.h>

#include "main/simulation.h"
#include "raylib.h"

static ant_logic_t ant_decision(ant_t *ant, double delta_time);

ant_t *ant_create(vector2d_t pos, vector2d_t spawn, Texture2D *texture, double rotation) {
  ant_t *ant = (ant_t *)malloc(sizeof(ant_t));
  if (!ant) {
    return NULL;
  }

  ant->texture = texture;
  ant->nearest_food = NULL;
  ant->net = NULL;
  ant->pos = pos;
  ant->spawn = spawn;
  ant->rotation = rotation;
  ant->has_food = false;
  ant->is_coliding = false;

  return ant;
}

void ant_update_nearest_food(ant_t *ant) {
  if (!ant) {
    return;
  }

  ant->nearest_food = NULL;

  circled_t detector_circle = ant_get_detector_circle(ant);
  double nearest_distance = DBL_MAX;
  for (int i = 0; i < food_list.length; i++) {
    food_t *food = dyn_arr_get(food_list, i);
    bool collide = circle_collide_circle(detector_circle, (circled_t){food->pos, food->detection_radius});
    if (!collide) {
      continue;
    }
    if (circle_collide_circle(detector_circle, (circled_t){food->pos, food->detection_radius})) {
      const double distance = v2d_distance_sqr(ant->pos, food->pos);
      if (distance < nearest_distance) {
        nearest_distance = distance;
        ant->nearest_food = food;
      }
    }
  }
}

void ant_run_update(ant_t *ant, ant_logic_t logic, double delta_time) {
  if (!ant) {
    return;
  }

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

ant_logic_t ant_train_update(ant_t *ant, double delta_time) {
  if (!ant) {
    return (ant_logic_t){ANT_STEP_ACTION, 0.0};
  }

  ant_logic_t logic = ant_decision(ant, delta_time);
  ant_run_update(ant, logic, delta_time);

  return logic;
}

void ant_draw(ant_t *ant) {
  if (!ant) {
    return;
  }

  const Vector2 center = {(ant->texture->width * ANT_SCALE) / 2.0f, (ant->texture->height * ANT_SCALE) / 2.0f};
  const Rectangle source = {0, 0, ant->texture->width, ant->texture->height};
  const Rectangle dest = {ant->pos.x, ant->pos.y, (ant->texture->width * ANT_SCALE),
                          (ant->texture->height * ANT_SCALE)};
  DrawTexturePro(*ant->texture, source, dest, center, ant->rotation * RAD2DEG, WHITE);

  const circled_t detector_circle = ant_get_detector_circle(ant);
  DrawCircleV(v2d_to_v2(detector_circle.center), detector_circle.radius,
              ant->has_food ? (Color){0, 255, 0, 128} : (Color){255, 0, 0, 128});
}

void ant_free(ant_t *ant) {
  if (!ant) {
    return;
  }
  if (ant->net) {
    free_neural_network(ant->net);
  }
  free(ant);
}

circled_t ant_get_detector_circle(ant_t *ant) {
  if (!ant) {
    return (circled_t){(vector2d_t){0.0, 0.0}, 0.0};
  }

  // Circle center position, accounting for rotation, position at head of the ant
  vector2d_t circle_pos = {
      ant->pos.x + (ANT_DETECTOR_OFFSET * ANT_SCALE) * cos(ant->rotation),
      ant->pos.y + (ANT_DETECTOR_OFFSET * ANT_SCALE) * sin(ant->rotation),
  };
  const circled_t circle = {circle_pos, ANT_DETECTOR_RADIUS * ANT_SCALE};
  return circle;
}

void ant_set_angle(ant_t *ant, double angle) {
  if (!ant) {
    return;
  }

  ant->rotation = constrain_angle(ant->rotation + angle);
}

bool ant_step(ant_t *ant, double delta_time) {
  if (!ant) {
    return false;
  }

  ant->pos.x += ANT_SPEED * cos(ant->rotation) * delta_time;
  ant->pos.y += ANT_SPEED * sin(ant->rotation) * delta_time;

  // Wrap around the world
  if (ant->pos.x < 0) {
    ant->pos.x += WORLD_W;
  } else if (ant->pos.x >= WORLD_W) {
    ant->pos.x -= WORLD_W;
  }
  if (ant->pos.y < 0) {
    ant->pos.y += WORLD_H;
  } else if (ant->pos.y >= WORLD_H) {
    ant->pos.y -= WORLD_H;
  }

  ant->is_coliding = false;
  return true;
}

bool ant_gather(ant_t *ant) {
  if (!ant) {
    return false;
  }

  if (!ant->nearest_food || ant->has_food) {
    return false;
  }

  // Check if the ant has reached the food
  if (circle_collide_point((circled_t){ant->nearest_food->pos, ant->nearest_food->radius}, ant->pos)) {
    food_grab(ant->nearest_food);
    ant->has_food = true;
    if (ant->nearest_food->amount <= 0) {
      for (int i = 0; i < food_list.length; i++) {
        if (dyn_arr_get(food_list, i) == ant->nearest_food) {
          dyn_arr_remove(food_list, i);
          break;
        }
      }

      food_free(ant->nearest_food);
    }

    return true;
  }

  return false;
}

bool ant_drop(ant_t *ant) {
  if (!ant) {
    return false;
  }

  // Check if the ant has reached the spawn point
  if (circle_collide_point((circled_t){ant->spawn, ANT_SPAWN_RADIUS}, ant->pos)) {
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
static ant_logic_t ant_decision(ant_t *ant, double delta_time) {
  if (!ant) {
    return (ant_logic_t){ANT_STEP_ACTION, 0.0};
  }

  double face_direction = 0.0;
  ant_action_t action = ANT_STEP_ACTION;

  // Collect food
  if (ant->has_food) {
    // Move towards the spawn point
    vector2d_t direction = v2d_normalize(v2d_subtract(ant->spawn, ant->pos));
    face_direction = atan2(direction.y, direction.x);

    if (circle_collide_point((circled_t){ant->spawn, ANT_SPAWN_RADIUS}, ant->pos)) {
      action = ANT_DROP_ACTION;
      face_direction = ant->rotation + M_PI_2;
    }
  }

  // Gather food
  else if (ant->nearest_food) {
    // Move towards the closest food
    vector2d_t direction = v2d_normalize(v2d_subtract(ant->nearest_food->pos, ant->pos));
    face_direction = atan2(direction.y, direction.x);

    if (circle_collide_point((circled_t){ant->nearest_food->pos, ant->nearest_food->radius}, ant->pos)) {
      action = ANT_GATHER_ACTION;
    }
  }

  else {
    if (ant->is_coliding) {
      face_direction = ant->rotation + M_PI * delta_time;
    } else {
      face_direction = ant->rotation + (M_PI / 64.0) * delta_time;
    }
  }

  return (ant_logic_t){action, constrain_angle(face_direction - ant->rotation)};
}