#pragma once
#ifndef GUI_H
#define GUI_H

#include "raylib.h"

#define TEXT_SIZE 20
#define TEXT_COLOR BLACK
#define BUTTON_COLOR (Color){220, 220, 220, 255}
#define BUTTON_HOVER_COLOR (Color){200, 200, 200, 255}
#define BUTTON_PRESS_COLOR (Color){170, 170, 170, 255}
#define BUTTON_ROUNDNESS 0.5f
#define CHECKBOX_BORDER_SIZE 2
#define CHECKBOX_BORDER_COLOR (Color){150, 150, 150, 255}
#define CHECKBOX_HOVER_COLOR BUTTON_HOVER_COLOR
#define CHECKBOX_PRESS_COLOR BUTTON_PRESS_COLOR
#define CHECKBOX_BACKGROUND_COLOR BUTTON_COLOR
#define CHECKBOX_CHECKED_COLOR (Color){0, 150, 0, 255}

bool gui_draw_button(Vector2 mouse_pos, Rectangle bounds, const char* text);
bool gui_draw_checkbox(Vector2 mouse_pos, Vector2 position, const char* text, bool checked);
void gui_draw_label(Vector2 position, const char* text);

#endif /* GUI_H */