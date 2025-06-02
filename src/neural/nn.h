#pragma once
#ifndef NEURAL_NETWORK_H
#define NEURAL_NETWORK_H

#include <stddef.h>
#include <stdio.h>

/**
 * @brief Artificial Neural Network (ANN) structure for a neural network.
 */
typedef struct {
  size_t num_hidden_layers;  // Number of hidden layers
  size_t total_neurons;      // Total number of neurons in the network
  size_t total_weights;      // Total number of weights in the network
  size_t *neuron_counts;     // Array containing the number of neurons in each layer
  double *output;            // 2D array: Output of each neuron in the network
  double *t_weights;         // Weights for the connections between neurons, stored as transposed
  double *bias;              // 2D array: Biases for each neuron in the network
} neural_network_t;

/**
 * @brief Create a neural network with the specified architecture.
 *
 * @param num_hidden_layers The number of hidden layers in the neural network.
 * @param neuron_counts_array An array containing the number of neurons in each layer, including input and output
 * layers.
 * @return A pointer to the created neural network, or \c NULL on failure.
 */
neural_network_t *create_neural_network(size_t num_hidden_layers, size_t neuron_counts_array[]);

/**
 * @brief Calculate the output of the neural network for a given input.
 *
 * @param network The neural network to use for calculation.
 * @param input The input data for the neural network.
 * @return A pointer to the output of the neural network.
 */
const double *run_neural_network(neural_network_t *network, const double *input);

/**
 * @brief Train the neural network using backpropagation.
 *
 * @param network The neural network to train.
 * @param m The number of training examples
 * @param inputs An array of input values for the training examples (input_neurons x m).
 * @param desired_output An array of desired output values for the training examples (output_neurons x m).
 * @param lr The learning rate for weight updates.
 * @return The cost of the training process, which can be used to evaluate the performance of the network.
 */
double train_neural_network(neural_network_t *network, size_t m, const double *inputs, const double *desired_outputs,
                            double lr);

/**
 * @brief Calculate the cost of the neural network's predictions.
 *
 * @param network The neural network to calculate the cost for.
 * @param m The number of training examples.
 * @param y The actual output values (ground truth).
 * @param y_hat The predicted output values from the neural network.
 * @return The calculated cost value.
 * @note Mean Squared Error (MSE): (1/2m) * Σ(y - y_hat)²
 */
double calculate_cost(neural_network_t *network, size_t m, const double *y, const double *y_hat);

/**
 * @brief Forward propagate A[layer], return next layer A[layer + 1]
 *
 * @param network The neural network to propagate through
 * @param layer The index of the layer to propagate from
 * @param m The number of training examples
 * @param A_in The input array for the current layer
 * @param A_out The output array for the next layer
 */
void forward_propagate_layer(neural_network_t *network, size_t layer, size_t m, const double *A_in, double *A_out);

/**
 * @brief Randomize the weights of the neural network.
 *
 * @param network The neural network to randomize.
 * @param min_weight The minimum weight value.
 * @param max_weight The maximum weight value.
 */
void randomize_weights(neural_network_t *network, double min_weight, double max_weight);

/**
 * @brief Randomize the biases of the neural network.
 *
 * @param network The neural network to randomize.
 * @param min_bias The minimum bias value.
 * @param max_bias The maximum bias value.
 */
void randomize_bias(neural_network_t *network, double min_bias, double max_bias);

/**
 * @brief Get the transposed weights for a specific in-layer in the neural network.
 * @param network The neural network.
 * @param layer The index of the in-layer to get weights for.
 * @return A pointer to the weights of the specified layer.
 *
 * Usage: double (*layer_weights)[network->neuron_counts[out_layer - 1]] = neural_layer_weights(network, out_layer);
 */
double (*neural_layer_t_weights(neural_network_t *network, size_t out_layer))[];

/**
 * @brief Print the structure and weights of the neural network.
 *
 * @param network The neural network to print.
 * @param fp The file pointer to write the neural network structure to.
 */
void write_neural_network(neural_network_t *network, FILE *fp);

/**
 * @brief Free the memory allocated for the neural network.
 *
 * @param network The neural network to destroy.
 */
void free_neural_network(neural_network_t *network);

#endif /* NEURAL_NETWORK_H */