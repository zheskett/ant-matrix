#include "food.h"

#include "raylib.h"

food_t* create_food(vector2d_t pos, double radius, double detection_radius, int amount) {
  food_t* food = (food_t*)malloc(sizeof(food_t));
  if (!food) {
    return NULL;
  }

  food->pos = pos;
  food->radius = radius;
  food->detection_radius = detection_radius;
  food->amount = amount;

  return food;
}

void update_food(food_t* food, double delta_time) {
  if (!food) {
    return;
  }

  // Not used yet
}

void draw_food(food_t* food) {
  if (!food) {
    return;
  }
  Vector2 ray_pos = v2d_to_v2(food->pos);

  DrawCircleV(ray_pos, (float)food->radius, PURPLE);
  DrawCircleLinesV(ray_pos, (float)food->detection_radius, DARKPURPLE);

  const char* text = TextFormat("%d", food->amount);
  const int font_size = 20;
  const Vector2 text_size = MeasureTextEx(GetFontDefault(), text, font_size, 2);
  DrawText(text, ray_pos.x - text_size.x / 2, ray_pos.y - text_size.y / 2, font_size, WHITE);
}

void destroy_food(food_t* food) {
  if (!food) {
    return;
  }

  free(food);
}

void grab_food(food_t* food) {
  if (!food) {
    return;
  }

  food->amount--;
}