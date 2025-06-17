/**
 * @file matrix.h
 * @brief Header file for matrix operations.
 *
 * This file defines the matrix_t structure and declares functions for matrix operations.
 */
#pragma once
#ifndef MATRIX_H
#define MATRIX_H

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

double matrix_get(const matrix_t *matrix, int row, int col);

#endif /* MATRIX_H */