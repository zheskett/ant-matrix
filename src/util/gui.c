#include "gui.h"

bool gui_draw_button(Vector2 mouse_pos, Rectangle bounds, const char* text) {
  bool clicked = false;
  if (CheckCollisionPointRec(mouse_pos, bounds)) {
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
      clicked = true;
    }
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
      DrawRectangleRounded(bounds, BUTTON_ROUNDNESS, 0, BUTTON_PRESS_COLOR);
    } else {
      DrawRectangleRounded(bounds, BUTTON_ROUNDNESS, 0, BUTTON_HOVER_COLOR);
    }
  } else {
    DrawRectangleRounded(bounds, BUTTON_ROUNDNESS, 0, BUTTON_COLOR);
  }

  const int text_width = MeasureText(text, TEXT_SIZE);
  const int text_height = TEXT_SIZE;
  Vector2 text_position = {bounds.x + (bounds.width - text_width) / 2, bounds.y + (bounds.height - text_height) / 2};
  DrawText(text, (int)text_position.x, (int)text_position.y, TEXT_SIZE, TEXT_COLOR);

  return clicked;
}

bool gui_draw_checkbox(Vector2 mouse_pos, Vector2 position, const char* text, bool checked) {
  Rectangle check_rectangle = {position.x, position.y, TEXT_SIZE, TEXT_SIZE};

  if (CheckCollisionPointRec(mouse_pos, check_rectangle)) {
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
      checked = !checked;
    }
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
      DrawRectangleRec(check_rectangle, CHECKBOX_PRESS_COLOR);
    } else {
      DrawRectangleRec(check_rectangle, CHECKBOX_HOVER_COLOR);
    }
  } else {
    DrawRectangleRec(check_rectangle, CHECKBOX_BACKGROUND_COLOR);
  }

  if (checked) {
    // Draw a checkmark
    Vector2 checkmark_start = {check_rectangle.x + CHECKBOX_BORDER_SIZE, check_rectangle.y + CHECKBOX_BORDER_SIZE};
    Vector2 checkmark_end = {check_rectangle.x + check_rectangle.width - CHECKBOX_BORDER_SIZE,
                             check_rectangle.y + check_rectangle.height - CHECKBOX_BORDER_SIZE};
    DrawLineEx(checkmark_start, checkmark_end, CHECKBOX_BORDER_SIZE, CHECKBOX_CHECKED_COLOR);
    DrawLineEx((Vector2){checkmark_start.x, checkmark_end.y}, (Vector2){checkmark_end.x, checkmark_start.y},
               CHECKBOX_BORDER_SIZE, CHECKBOX_CHECKED_COLOR);
  }
  DrawRectangleLinesEx(check_rectangle, CHECKBOX_BORDER_SIZE, CHECKBOX_BORDER_COLOR);

  const int text_width = MeasureText(text, TEXT_SIZE);
  Vector2 text_position = {check_rectangle.x + check_rectangle.width + 5,
                           check_rectangle.y + (check_rectangle.height - TEXT_SIZE) / 2};
  DrawText(text, (int)text_position.x, (int)text_position.y, TEXT_SIZE, TEXT_COLOR);

  return checked;
}

void gui_draw_label(Vector2 position, const char* text) {
  DrawText(text, (int)position.x, (int)position.y, TEXT_SIZE, TEXT_COLOR);
}