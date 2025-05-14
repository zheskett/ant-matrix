#include "ant.h"
#include <stdlib.h>

ant_t* create_ant(float x, float y, float rotation) {
  ant_t* ant = (ant_t*) malloc(sizeof(ant_t));
  if (!ant) {
    return NULL;
  }

  ant->texture = LoadTexture("assets/ant.png");
  ant->pos = (Vector2){ x, y };
  ant->rotation = rotation;

  return ant;
}

void update_ant(ant_t* ant, float delta_time) {
  if (ant) {
    ant->rotation += 90.0f * delta_time; // Rotate the ant
    if (ant->rotation >= 360.0f) {
      ant->rotation -= 360.0f; // Keep rotation within 0-360 degrees
    }
  }
}

void draw_ant(ant_t* ant) {
  Vector2 center = { ant->texture.width / 2, ant->texture.height / 2 };
  if (ant) {
    DrawTexturePro(ant->texture, (Rectangle) { 0, 0, ant->texture.width, ant->texture.height },
      (Rectangle) {
      ant->pos.x, ant->pos.y, ant->texture.width, ant->texture.height
    },
      center,
      ant->rotation, WHITE);
  }
}

void destroy_ant(ant_t* ant) {
  if (ant) {
    UnloadTexture(ant->texture);
    free(ant);
  }
}