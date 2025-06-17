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

#include "neural/nn.h"
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
#define NETWORK_LAYER_SPACING 100.0f
#define NETWORK_NEURON_SPACING 20.0f
#define NETWORK_NEURON_SIZE 10.0f
#define NETWORK_CONNECTION_WIDTH 1.0f
#define PROGRESS_BAR_COLOR (Color){200, 0, 200, 255}
#define PROGRESS_BAR_BACKGROUND_COLOR GRAY
#define PROGRESS_BAR_ROUNDNESS 0.3f
#define PROGRESS_BAR_OUTLINE_COLOR WHITE
#define PROGRESS_BAR_OUTLINE_WIDTH 8.0f

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

/**
 * @brief Draws a label centered at the specified position with the given text.
 *
 * @param position The position to center the label at.
 * @param text The text to display in the label.
 */
void gui_draw_label_centered(Vector2 position, const char *text);

/**
 * @brief Draws a neural network visualization at the specified position.
 *
 * @param position The position to draw the neural network visualization.
 * @param network The neural network to visualize.
 * @param draw_output Whether to draw the output values of the neurons or the biases.
 */
void gui_draw_neural_network(Vector2 position, neural_network_t *network, bool draw_output);

/**
 * @brief Draws a progress bar at the specified bounds with the given progress.
 *
 * @param bounds The rectangle bounds of the progress bar.
 * @param progress The progress value (0.0 to 1.0).
 */
void gui_draw_progress_bar(Rectangle bounds, float progress, const char *text);

#endif /* GUI_H */