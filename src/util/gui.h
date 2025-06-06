/**
 * @file gui.h
 * @author Zachary Heskett (zheskett@gmail.com)
 * @brief GUI utilities for the ant simulation program.
 *
 * @copyright Copyright (c) 2025
 *
 */
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

/**
 * @brief Draws a button with the specified text and returns whether it was clicked.
 *
 * @param mouse_pos The current mouse position.
 * @param bounds The rectangle bounds of the button.
 * @param text The text to display on the button.
 * @return true if the button was clicked, false otherwise.
 */
bool gui_draw_button(Vector2 mouse_pos, Rectangle bounds, const char *text);

/**
 * @brief Draws a checkbox with the specified text and returns whether it was checked.
 *
 * @param mouse_pos The current mouse position.
 * @param position The position of the checkbox.
 * @param text The text to display next to the checkbox.
 * @param checked The initial state of the checkbox.
 * @return true if the checkbox was checked, false otherwise.
 */
bool gui_draw_checkbox(Vector2 mouse_pos, Vector2 position, const char *text, bool checked);

/**
 * @brief Draws a label at the specified position with the given text.
 *
 * @param position The position to draw the label at.
 * @param text The text to display in the label.
 */
void gui_draw_label(Vector2 position, const char *text);

#endif /* GUI_H */