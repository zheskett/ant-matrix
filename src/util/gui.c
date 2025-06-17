#include "gui.h"

#include <math.h>

static Color get_neuron_color(double value, bool is_output);
static Color get_weight_color(double weight);

bool gui_draw_button(Vector2 mouse_pos, Rectangle bounds, const char *text) {
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

bool gui_draw_checkbox(Vector2 mouse_pos, Vector2 position, const char *text, bool checked) {
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

  Vector2 text_position = {check_rectangle.x + check_rectangle.width + 5,
                           check_rectangle.y + (check_rectangle.height - TEXT_SIZE) / 2};
  DrawText(text, (int)text_position.x, (int)text_position.y, TEXT_SIZE, TEXT_COLOR);

  return checked;
}

void gui_draw_label(Vector2 position, const char *text) {
  DrawText(text, (int)position.x, (int)position.y, TEXT_SIZE, TEXT_COLOR);
}

void gui_draw_label_centered(Vector2 position, const char *text) {
  const int text_width = MeasureText(text, TEXT_SIZE);
  Vector2 centered_position = {position.x - text_width / 2.0f, position.y};
  gui_draw_label(centered_position, text);
}

void gui_draw_neural_network(Vector2 position, neural_network_t *network, bool draw_output) {
  if (!network) {
    return;
  }

  float max_height = 0.0f;
  for (int i = 0; i < network->num_layers; i++) {
    max_height = fmaxf(max_height, network->neuron_counts[i] * NETWORK_NEURON_SPACING);
  }

  for (int i = 1; i < network->num_layers; i++) {
    matrix_t weights = neural_layer_weightsT(network, i);
    int in_size = weights.cols;
    int out_size = weights.rows;

    const int layer_height_in = in_size * NETWORK_NEURON_SPACING;
    float y_offset_in = (max_height - layer_height_in) / 2.0f;
    const int layer_height_out = out_size * NETWORK_NEURON_SPACING;
    float y_offset_out = (max_height - layer_height_out) / 2.0f;

    for (int k = 0; k < out_size; k++) {
      for (int j = 0; j < in_size; j++) {
        Vector2 prev_neuron_pos = {position.x + (i - 1) * NETWORK_LAYER_SPACING + NETWORK_NEURON_SIZE,
                                   position.y + j * NETWORK_NEURON_SPACING + y_offset_in + NETWORK_NEURON_SIZE / 2.0f};
        Vector2 neuron_pos = {position.x + i * NETWORK_LAYER_SPACING,
                              position.y + k * NETWORK_NEURON_SPACING + y_offset_out + NETWORK_NEURON_SIZE / 2.0f};

        const double weight = matrix_get(&weights, k, j);
        DrawLineEx(prev_neuron_pos, neuron_pos, NETWORK_CONNECTION_WIDTH, get_weight_color(weight));
      }
    }
  }

  for (int i = 0; i < network->num_layers; i++) {
    const int layer_height = network->neuron_counts[i] * NETWORK_NEURON_SPACING;
    float y_offset = (max_height - layer_height) / 2.0f;
    for (int j = 0; j < network->neuron_counts[i]; j++) {
      Rectangle neuron = {position.x + i * NETWORK_LAYER_SPACING, position.y + j * NETWORK_NEURON_SPACING + y_offset,
                          NETWORK_NEURON_SIZE, NETWORK_NEURON_SIZE};
      Color neuron_color = draw_output ? get_neuron_color(network->output[i].data[j], true)
                                       : (i != 0 ? get_neuron_color(network->bias[i].data[j], false) : DARKBLUE);
      DrawRectangleRec(neuron, neuron_color);
    }
  }
}

void gui_draw_progress_bar(Rectangle bounds, float progress, const char *text) {
  if (progress < 0.0f || progress > 1.0f) {
    return;
  }
  // Draw the outline
  const float half_width = PROGRESS_BAR_OUTLINE_WIDTH / 2.0f;
  const Rectangle outline_bounds = {bounds.x - half_width, bounds.y - half_width,
                                    bounds.width + PROGRESS_BAR_OUTLINE_WIDTH,
                                    bounds.height + PROGRESS_BAR_OUTLINE_WIDTH};
  DrawRectangleRounded(outline_bounds, PROGRESS_BAR_ROUNDNESS, 0, PROGRESS_BAR_OUTLINE_COLOR);

  // Draw the background
  DrawRectangleRounded(bounds, PROGRESS_BAR_ROUNDNESS, 0, PROGRESS_BAR_BACKGROUND_COLOR);

  // Draw the filled part
  const Rectangle filled_rect = {bounds.x, bounds.y, bounds.width * progress, bounds.height};
  DrawRectangleRounded(filled_rect, PROGRESS_BAR_ROUNDNESS, 0, PROGRESS_BAR_COLOR);

  // Draw the text
  if (text) {
    const int text_width = MeasureText(text, TEXT_SIZE);
    const int text_height = TEXT_SIZE;
    Vector2 text_position = {bounds.x + (bounds.width - text_width) / 2, bounds.y + (bounds.height - text_height) / 2};
    DrawText(text, (int)text_position.x, (int)text_position.y, TEXT_SIZE, TEXT_COLOR);
  }
}

static Color get_neuron_color(double value, bool is_output) {
  value = is_output ? dec(fmax(0, fmin(1.0, value))) : tanh(value);

  // Map the value to a color with gradient from red to yellow to green
  if (value < 0.0) {
    return (Color){255, (unsigned char)((value + 1.0) * 255), 0, 255};
  }

  return (Color){(unsigned char)((1.0 - value) * 255), 255, 0, 255};
}

static Color get_weight_color(double weight) {
  weight = tanh(weight);
  unsigned char color_value = (unsigned char)((weight * 0.5 + 0.5) * 255);

  // Map the weight to a color black to white
  return (Color){color_value, color_value, color_value, (unsigned char)(fmax(fabs(weight), 0.1) * 255)};
}