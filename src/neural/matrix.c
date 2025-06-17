#include "neural/matrix.h"

#include <math.h>
#include <string.h>

double matrix_get(const matrix_t *matrix, int row, int col) {
  if (!matrix || row < 0 || row >= matrix->rows || col < 0 || col >= matrix->cols) {
    return NAN;
  }
  return matrix->data[row * matrix->cols + col];
}

void matrix_multiply(const matrix_t *A, const matrix_t *B, matrix_t *result) {
  if (!A || !B || !result || A->cols != B->rows || result->rows != A->rows || result->cols != B->cols) {
    return;
  }

  // Initialize result matrix to zero
  memset(result->data, 0, result->rows * result->cols * sizeof(double));
  matrix_multiply_append(A, B, result);
}

void matrix_multiply_append(const matrix_t *A, const matrix_t *B, matrix_t *result) {
  if (!A || !B || !result || A->cols != B->rows || result->rows != A->rows || result->cols != B->cols) {
    return;
  }

  for (int i = 0; i < A->rows; i++) {
    const double *A_i = A->data + (i * A->cols);
    double *result_i = result->data + (i * result->cols);
    for (int j = 0; j < B->rows; j++) {
      const double *B_j = B->data + (j * B->cols);
      const double A_ij = A_i[j];
      for (int k = 0; k < B->cols; k++) {
        result_i[k] += A_ij * B_j[k];
      }
    }
  }
}

void matrix_multiply_transformA(const matrix_t *A, const matrix_t *B, matrix_t *result, bool zero_init) {
  if (!A || !B || !result || A->rows != B->rows || result->rows != A->cols || result->cols != B->cols) {
    return;
  }
  if (zero_init) {
    // Initialize result matrix to zero
    memset(result->data, 0, result->rows * result->cols * sizeof(double));
  }

  for (int j = 0; j < B->rows; j++) {
    const double *A_j = A->data + (j * A->cols);
    const double *B_j = B->data + (j * B->cols);
    for (int i = 0; i < A->cols; i++) {
      const double A_ij = A_j[i];
      double *result_i = result->data + (i * result->cols);
      for (int k = 0; k < B->cols; k++) {
        result_i[k] += A_ij * B_j[k];
      }
    }
  }
}

void matrix_multiply_transformB(const matrix_t *A, const matrix_t *B, matrix_t *result, bool zero_init) {
  if (!A || !B || !result || A->cols != B->cols || result->rows != A->rows || result->cols != B->rows) {
    return;
  }
  for (int i = 0; i < A->rows; i++) {
    const double *A_i = A->data + (i * A->cols);
    double *result_i = result->data + (i * result->cols);
    for (int j = 0; j < B->rows; j++) {
      const double *B_j = B->data + (j * B->cols);
      double sum = 0.0;
      for (int k = 0; k < A->cols; k++) {
        sum += A_i[k] * B_j[k];
      }
      result_i[j] = zero_init ? sum : result_i[j] + sum;
    }
  }
}

double vector_get(const vector_t *vector, int row) {
  if (!vector || row < 0 || row >= vector->rows) {
    return NAN;
  }
  return vector->data[row];
}