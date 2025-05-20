#include "ant.h"

#include <math.h>
#include <stdlib.h>

#include "main.h"
#include "raylib.h"
#include "util/definitions.h"

ant_t* create_ant(Vector2 pos, Texture2D* texture, float rotation) {
  ant_t* ant = (ant_t*)malloc(sizeof(ant_t));
  if (!ant) {
    return NULL;
  }

  ant->texture = texture;
  ant->pos = pos;
  ant->rotation = rotation;
  ant->has_food = false;
  ant->state = ANT_IDLE;

  return ant;
}

void update_ant(ant_t* ant, float delta_time) {
  if (!ant) {
    return;
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

  // Gather food
  if (!(ant->has_food)) {
    Circle detector_circle = get_ant_detector_circle(ant);
    for (int i = 0; i < food_vec.length; i++) {
      food_t* food = food_vec.data[i];
      if (CheckCollisionCircles(detector_circle.center, detector_circle.radius, food->pos, food->detection_radius)) {
        ant->has_food = true;
        food->amount--;
        if (food->amount <= 0) {
          destroy_food(food);
          vec_splice(&food_vec, i, 1);
        }
        break;
      }
    }
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
  DrawTexturePro(*ant->texture, source, dest, center, ant->rotation, WHITE);

  Circle detector_circle = get_ant_detector_circle(ant);
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
      ant->pos.x + (ANT_DETECTOR_OFFSET * ANT_SCALE) * cosf(DEG2RAD * ant->rotation),
      ant->pos.y + (ANT_DETECTOR_OFFSET * ANT_SCALE) * sinf(DEG2RAD * ant->rotation),
  };
  const Circle circle = {circle_pos, ANT_DETECTOR_RADIUS * ANT_SCALE};
  return circle;
}