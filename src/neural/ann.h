#pragma once

/**
 * @brief Artificial Neural Network (ANN) structure for a neural network.
 */
typedef struct {
  int num_hidden_layers;  // Number of hidden layers
  int *neuron_counts;     // Array containing the number of neurons in each layer
  double *output;         // 2D array: Output of each neuron in the network
  double *weights;        // 3D array: Weights for the connections between neurons
} neural_network_t;

/**
 * @brief Sigmoid activation function for neural networks.
 *
 * @param x The input value to the sigmoid function.
 * @return The output value after applying the sigmoid function.
 */
double neural_sigmoid(double x);

/**
 * @brief Create a neural network with the specified architecture.
 *
 * @param num_inputs The number of input neurons.
 * @param num_hidden_layers The number of hidden layers.
 * @param num_hidden_array An array containing the number of neurons in each hidden layer.
 * @param num_outputs The number of output neurons.
 * @return A pointer to the created neural network, or \c NULL on failure.
 */
neural_network_t *create_neural_network(int num_hidden_layers, int neuron_counts_array[]);

/**
 * @brief Randomize the weights of the neural network.
 *
 * @param network The neural network to randomize.
 * @param min_weight The minimum weight value.
 * @param max_weight The maximum weight value.
 */
void randomize_weights(neural_network_t *network, double min_weight, double max_weight);

/**
 * @brief Get the weights for a specific in-layer in the neural network. \n
 *
 * @param network The neural network.
 * @param layer The index of the in-layer to get weights for.
 * @return A pointer to the weights of the specified layer.
 *
 * Usage: double (*layer_weights)[network->neuron_counts[layer + 1]] = neural_layer_weights(network, layer);
 */
double (*neural_layer_weights(neural_network_t *network, int layer))[];

/**
 * @brief Print the structure and weights of the neural network.
 *
 * @param network The neural network to print.
 */
void print_neural_network(neural_network_t *network);

/**
 * @brief Free the memory allocated for the neural network.
 *
 * @param network The neural network to destroy.
 */
void free_neural_network(neural_network_t *network);