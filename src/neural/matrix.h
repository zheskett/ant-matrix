/**
 * @file matrix.h
 * @brief Header file for matrix operations.
 *
 * This file defines the matrix_t structure and declares functions for matrix operations.
 */
#pragma once
#ifndef MATRIX_H
#define MATRIX_H

#include <stdbool.h>

/**
 * @brief Matrix structure for representing a matrix with dynamic data.
 */
typedef struct {
  double *data; /**< Pointer to the data of the matrix */
  int rows;     /**< Number of rows in the matrix (n) */
  int cols;     /**< Number of columns in the matrix (m) */
} matrix_t;

/**
 * @brief Vector structure for representing a vector with dynamic data.
 */
typedef struct {
  double *data; /**< Pointer to the data of the vector */
  int rows;     /**< Number of rows in the vector (n) */
} vector_t;

/**
 * @brief Get the value at a specific row and column in a matrix.
 *
 * @param matrix Pointer to the matrix.
 * @param row The row index (0-based).
 * @param col The column index (0-based).
 * @return The value at the specified row and column, or NaN if out of bounds.
 */
double matrix_get(const matrix_t *matrix, int row, int col);

/**
 * @brief Multiply two matrices and store the result in a third matrix.
 *
 * @param A Pointer to the first matrix (n x m).
 * @param B Pointer to the second matrix (m x p).
 * @param result Pointer to the result matrix (n x p), must be preallocated.
 */
void matrix_multiply(const matrix_t *A, const matrix_t *B, matrix_t *result);

/**
 * @brief Multiply two matrices and store the result in a third matrix where all values are initially set to 0 or a
 * predetermined value.
 *
 * @param A Pointer to the first matrix (n x m).
 * @param B Pointer to the second matrix (m x p).
 * @param result Pointer to the result matrix (n x p), must be preallocated.
 * @note result MUST HAVE ALL MEMORY ALLOCATED AND SET TO ZERO or predetermined value.
 */
void matrix_multiply_append(const matrix_t *A, const matrix_t *B, matrix_t *result);

/**
 * @brief Multiply two matrices with the first matrix transposed and store the result in a third matrix.
 *
 * @param A Pointer to the first matrix (m x n).
 * @param B Pointer to the second matrix (m x p).
 * @param zero_init If true, initialize the result matrix to zero before multiplication.
 * @param result Pointer to the result matrix (n x p), must be preallocated.
 */
void matrix_multiply_transformA(const matrix_t *A, const matrix_t *B, matrix_t *result, bool zero_init);

/**
 * @brief Multiply two matrices with the second matrix transposed and store the result in a third matrix.
 *
 * @param A Pointer to the first matrix (n x m).
 * @param B Pointer to the second matrix (p x m).
 * @param zero_init If true, initialize the result matrix to zero before multiplication.
 * @param result Pointer to the result matrix (n x p), must be preallocated.
 */
void matrix_multiply_transformB(const matrix_t *A, const matrix_t *B, matrix_t *result, bool zero_init);

/**
 * @brief Get the value at a specific row in a vector.
 *
 * @param vector Pointer to the vector.
 * @param row The row index (0-based).
 * @return The value at the specified row, or NaN if out of bounds.
 */
double vector_get(const vector_t *vector, int row);

#endif /* MATRIX_H */