#pragma once

/**
 * @brief Artificial Neural Network (ANN) structure for a neural network.
 */
typedef struct {
  int num_hidden_layers;  // Number of hidden layers
  int total_neurons;      // Total number of neurons in the network
  int total_weights;      // Total number of weights in the network
  int *neuron_counts;     // Array containing the number of neurons in each layer
  double *output;         // 2D array: Output of each neuron in the network
  double *t_weights;      // Weights for the connections between neurons, stored as transposed
  double *bias;           // 2D array: Biases for each neuron in the network
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
 * @brief Calculate the output of the neural network for a given input.
 *
 * @param network The neural network to use for calculation.
 * @param input The input data for the neural network.
 * @return A pointer to the output of the neural network.
 */
const double *neural_network_output(neural_network_t *network, double *input);

/**
 * @brief Train the neural network using backpropagation.
 *
 * @param network The neural network to train.
 * @param m The number of training examples (size of inputs and desired_outputs).
 * @param inputs An array of input values for the training examples (input_neurons x ).
 * @param desired_outputs
 * @param learning_rate
 */
void train_neural_network(neural_network_t *network, int m, double *inputs, double *desired_outputs,
                          double learning_rate);

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
double (*neural_layer_t_weights(neural_network_t *network, int out_layer))[];

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