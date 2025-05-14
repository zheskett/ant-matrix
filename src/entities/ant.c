#include "ant.h"

#include <math.h>
#include <stdlib.h>

#include "raylib.h"
#include "util/uitl.h"

ant_t* create_ant(float x, float y, float rotation) {
  ant_t* ant = (ant_t*)malloc(sizeof(ant_t));
  if (!ant) {
    return NULL;
  }

  ant->texture = LoadTexture("assets/ant.png");
  ant->pos = (Vector2){x, y};
  ant->rotation = rotation;

  return ant;
}

void update_ant(ant_t* ant, float delta_time) {
  if (!ant) {
    return;
  }
  ant->rotation += 90.0f * delta_time;  // Rotate the ant
  if (ant->rotation >= 360.0f) {
    ant->rotation -= 360.0f;  // Keep rotation within 0-360 degrees
  }
}

void draw_ant(ant_t* ant) {
  if (!ant) {
    return;
  }

  Vector2 center = {(ant->texture.width * ANT_SCALE) / 2.0f, (ant->texture.height * ANT_SCALE) / 2.0f};
  Rectangle source = {0, 0, ant->texture.width, ant->texture.height};
  Rectangle dest = {ant->pos.x, ant->pos.y, (ant->texture.width * ANT_SCALE), (ant->texture.height * ANT_SCALE)};
  DrawTexturePro(ant->texture, source, dest, center, ant->rotation, WHITE);

  Circle detector_circle = get_ant_detector_circle(ant);
  DrawCircleV(detector_circle.center, detector_circle.radius, (Color){255, 0, 0, 128});
}

void destroy_ant(ant_t* ant) {
  if (ant) {
    UnloadTexture(ant->texture);
    free(ant);
  }
}

Circle get_ant_detector_circle(ant_t* ant) {
  if (!ant) {
    return (Circle){(Vector2){0.0f, 0.0f}, 0.0f};
  }

  // Circle center position, accounting for rotation, position at head of the ant
  Vector2 circle_pos = {
      ant->pos.x + (ANT_DETECTOR_OFFSET * ANT_SCALE) * sinf(DEG2RAD * ant->rotation),
      ant->pos.y + (ANT_DETECTOR_OFFSET * ANT_SCALE) * -cosf(DEG2RAD * ant->rotation),
  };
  Circle circle = {circle_pos, ANT_DETECTOR_RADIUS * ANT_SCALE};
  return circle;
}