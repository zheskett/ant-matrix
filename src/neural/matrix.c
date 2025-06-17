#include "neural/matrix.h"

double matrix_get(const matrix_t *matrix, int row, int col) {
  if (!matrix || row < 0 || row >= matrix->rows || col < 0 || col >= matrix->cols) {
    return 0.0; // Return 0 for invalid access
  }
  return matrix->data[row * matrix->cols + col];
}