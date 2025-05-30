#pragma once

/**
 * @brief Artificial Neural Network (ANN) structure for a neural network.
 */
typedef struct {
  int num_inputs;         // Number of input neurons
  int num_hidden_layers;  // Number of hidden layers
  int num_outputs;        // Number of output neurons
  int *num_hidden;        // Array containing the number of neurons in each hidden layer
  double **output;        // Output of each neuron in the network
  double ***weights;      // Weights for the connections between neurons
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
neural_network_t *create_neural_network(int num_inputs, int num_hidden_layers, int num_hidden_array[], int num_outputs);

/**
 * @brief Randomize the weights of the neural network.
 *
 * @param network The neural network to randomize.
 * @param min_weight The minimum weight value.
 * @param max_weight The maximum weight value.
 */
void randomize_weights(neural_network_t *network, double min_weight, double max_weight);

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