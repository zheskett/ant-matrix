/**
 * @file nn.h
 * @author Zachary Heskett (zheskett@gmail.com)
 * @brief Header file for the neural network implementation in the ant simulation program.
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef NEURAL_NETWORK_H
#define NEURAL_NETWORK_H

#include "neural/matrix.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define INITIAL_DATA_SIZE 8

/**
 * @brief Artificial Neural Network (ANN) structure for a neural network.
 */
typedef struct {
  int *neuron_counts; /**< Array containing the number of neurons in each layer */
  vector_t *output;   /**< Output of each neuron in the network, array indexed by layer */
  matrix_t *weightsT; /**< Weights for the connections between neurons, stored as transposed, array indexed by layer */
  vector_t *bias;     /**< Biases for each neuron in the network, array indexed by layer */
  void *data;         /**< Pointer to the data used for training and inference */
  matrix_t Q_data;    /**< Q matrix for storing data used in training and inference */
  size_t data_size;   /**< Size of the data used for training and inference */
  int num_hidden_layers; /**< Number of hidden layers */
  int num_layers;        /**< Total number of layers in the network (input + hidden + output) */
  int total_neurons;     /**< Total number of neurons in the network */
  int total_weights;     /**< Total number of weights in the network */
} neural_network_t;

double enc(double x);
double dec(double x);

/**
 * @brief Create a neural network with the specified architecture.
 *
 * @param num_hidden_layers The number of hidden layers in the neural network.
 * @param neuron_counts_array An array containing the number of neurons in each layer, including input and output
 * layers.
 * @return A pointer to the created neural network, or NULL on failure.
 */
neural_network_t *neural_create(int num_hidden_layers, const int neuron_counts_array[]);

/**
 * @brief Calculate the output of the neural network for a given input.
 *
 * @param network The neural network to use for calculation.
 * @param input The input data for the neural network.
 * @return A pointer to the output of the neural network.
 */
const vector_t *neural_run(neural_network_t *network, const vector_t *input);

/**
 * @brief Train the neural network using backpropagation.
 *
 * @param network The neural network to train.
 * @param inputs An matrix of input values for the training examples (m x input_neurons).
 * @param desired_output An matrix of desired output values for the training examples (m x output_neurons).
 * @param lr The learning rate for weight updates.
 * @return The cost of the training process, which can be used to evaluate the performance of the network.
 */
double neural_train(neural_network_t *network, const matrix_t *inputs, const matrix_t *desired_outputs, double lr);

/**
 * @brief Randomize the weights of the neural network.
 *
 * @param network The neural network to randomize.
 * @param min_weight The minimum weight value.
 * @param max_weight The maximum weight value.
 */
void neural_randomize_weights(neural_network_t *network, double min_weight, double max_weight);

/**
 * @brief Randomize the biases of the neural network.
 *
 * @param network The neural network to randomize.
 * @param min_bias The minimum bias value.
 * @param max_bias The maximum bias value.
 */
void neural_randomize_bias(neural_network_t *network, double min_bias, double max_bias);

/**
 * @brief Get the transposed weights for a specific in-layer in the neural network.
 * @param network The neural network.
 * @param layer The index of the in-layer to get weights for.
 * @return The weights of the specified layer.
 */
matrix_t neural_layer_weightsT(neural_network_t *network, int out_layer);

/**
 * @brief Print the structure and weights of the neural network.
 *
 * @param network The neural network to print.
 * @param fp The file pointer to write the neural network structure to.
 */
void neural_print(neural_network_t *network, FILE *fp);

// /**
//  * @brief Write the neural network to a file.
//  *
//  * @param network The neural network to write.
//  * @param filename The name of the file to write the neural network to.
//  * @return True if the write operation was successful, false otherwise.
//  */
// bool neural_write(neural_network_t *network, const char *filename);

// /**
//  * @brief Read a neural network from a file.
//  *
//  * @param filename The name of the file to read the neural network from.
//  * @return A pointer to the read neural network, or NULL on failure.
//  */
// neural_network_t *neural_read(const char *filename);

/**
 * @brief Free the memory allocated for the neural network.
 *
 * @param network The neural network to destroy.
 */
void neural_free(neural_network_t *network);

#endif /* NEURAL_NETWORK_H */