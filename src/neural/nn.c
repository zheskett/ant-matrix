#include "neural/nn.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/util.h"

static void calculate_output_layer(neural_network_t *network, size_t output_layer);
static void forward_propagate(neural_network_t *network, size_t layer, size_t m, const double *A_in, double *A_out);
static size_t neural_layer_offset(neural_network_t *network, size_t layer);

static inline double neural_sigmoid(double x) { return 1.0 / (1.0 + exp(-x)); }

neural_network_t *create_neural_network(size_t num_hidden_layers, size_t neuron_counts_array[]) {
  num_hidden_layers = MAX(0, num_hidden_layers);
  neural_network_t *network = malloc(sizeof(neural_network_t));
  if (!network) {
    return NULL;
  }

  network->num_hidden_layers = num_hidden_layers;
  network->total_neurons = 0;
  network->total_weights = 0;
  network->neuron_counts = NULL;
  network->output = NULL;
  network->bias = NULL;
  network->t_weights = NULL;

  size_t weights_size = 0;
  size_t total_neurons = neuron_counts_array[0];
  for (size_t i = 0; i < num_hidden_layers + 1; i++) {
    const size_t in_size = neuron_counts_array[i];
    const size_t out_size = neuron_counts_array[i + 1];
    if (in_size <= 0 || out_size <= 0) {
      free_neural_network(network);
      return NULL;
    }

    total_neurons += out_size;
    weights_size += in_size * out_size;
  }
  network->total_neurons = total_neurons;
  network->total_weights = weights_size;

  network->neuron_counts = calloc(num_hidden_layers + 2, sizeof(*network->neuron_counts));
  if (!network->neuron_counts) {
    free_neural_network(network);
    return NULL;
  }
  memcpy(network->neuron_counts, neuron_counts_array, (num_hidden_layers + 2) * sizeof(*network->neuron_counts));

  network->output = calloc(total_neurons, sizeof(*network->output));
  if (!network->output) {
    free_neural_network(network);
    return NULL;
  }

  network->t_weights = calloc(weights_size, sizeof(*network->t_weights));
  if (!network->t_weights) {
    free_neural_network(network);
    return NULL;
  }

  network->bias = calloc(total_neurons, sizeof(*network->bias));
  if (!network->bias) {
    free_neural_network(network);
    return NULL;
  }

  return network;
}

const double *run_neural_network(neural_network_t *network, double *input) {
  memcpy(network->output, input, network->neuron_counts[0] * sizeof(double));
  for (size_t i = 1; i < network->num_hidden_layers + 2; i++) {
    calculate_output_layer(network, i);
  }

  return (const double *)(network->output + neural_layer_offset(network, network->num_hidden_layers + 1));
}

void train_neural_network(neural_network_t *network, size_t m, double *inputs, double *desired_outputs, double lr) {}

void randomize_weights(neural_network_t *network, double min_weight, double max_weight) {
  if (!network || !network->t_weights) {
    return;
  }

  for (size_t i = 0; i < network->total_weights; i++) {
    network->t_weights[i] = min_weight + (max_weight - min_weight) * ((double)rand() / RAND_MAX);
  }
}

void randomize_bias(neural_network_t *network, double min_bias, double max_bias) {
  if (!network || !network->bias) {
    return;
  }

  for (size_t i = 0; i < network->total_neurons; i++) {
    network->bias[i] = min_bias + (max_bias - min_bias) * ((double)rand() / RAND_MAX);
  }
}

double (*neural_layer_t_weights(neural_network_t *network, size_t out_layer))[] {
  size_t in_size = network->neuron_counts[out_layer - 1];
  size_t offset = 0;
  for (size_t i = 0; i < out_layer - 1; i++) {
    size_t out_size_prev = network->neuron_counts[i + 1];
    size_t in_size_prev = network->neuron_counts[i];

    offset += out_size_prev * in_size_prev;
  }
  return (double (*)[in_size])(network->t_weights + offset);
}

void write_neural_network(neural_network_t *network, FILE *fp) {
  if (!network || !fp) {
    return;
  }

  fprintf(fp, "Neural Network Structure:\n");
  fprintf(fp, "Inputs: %zu\n", network->neuron_counts[0]);
  fprintf(fp, "Hidden Layers: %d\n", network->num_hidden_layers);
  for (size_t i = 0; i < network->num_hidden_layers; i++) {
    fprintf(fp, "Hidden Layer %d: %d neurons\n", i + 1, network->neuron_counts[i + 1]);
  }
  fprintf(fp, "Outputs: %zu\n", network->neuron_counts[network->num_hidden_layers + 1]);

  fprintf(fp, "\nT_Weights:");
  for (size_t i = 0; i < network->num_hidden_layers + 1; i++) {
    fprintf(fp, "\nLayer %zu-%zu:\n", i, i + 1);
    double (*layer_weights)[network->neuron_counts[i]] = neural_layer_t_weights(network, i + 1);
    for (size_t j = 0; j < network->neuron_counts[i + 1]; j++) {
      fprintf(fp, "[");
      for (size_t k = 0; k < network->neuron_counts[i]; k++) {
        if (k > 0) {
          fprintf(fp, ", ");
        }
        fprintf(fp, "%.3f", layer_weights[k][j]);
      }
      fprintf(fp, "]\n");
    }
  }

  fprintf(fp, "\nBiases:\n");
  for (size_t i = 0; i < network->num_hidden_layers + 2; i++) {
    fprintf(fp, "Layer %d: ", i);
    double *bias = network->bias + neural_layer_offset(network, i);
    for (size_t j = 0; j < network->neuron_counts[i]; j++) {
      if (j > 0) {
        fprintf(fp, ", ");
      }
      fprintf(fp, "%.3f", bias[j]);
    }
    fprintf(fp, "\n");
  }

  fflush(fp);
  if (ferror(fp)) {
    fprintf(stderr, "Error writing neural network to file.\n");
  }
}

void free_neural_network(neural_network_t *network) {
  if (!network) {
    return;
  }

  if (network->neuron_counts) {
    free(network->neuron_counts);
  }
  if (network->output) {
    free(network->output);
  }
  if (network->t_weights) {
    free(network->t_weights);
  }
  if (network->bias) {
    free(network->bias);
  }
  network->neuron_counts = NULL;
  network->output = NULL;
  network->t_weights = NULL;
  network->bias = NULL;
  network->num_hidden_layers = 0;
  network->total_neurons = 0;
  network->total_weights = 0;
  free(network);
}

static size_t neural_layer_offset(neural_network_t *network, size_t layer) {
  size_t offset = 0;
  for (size_t i = 0; i < layer; i++) {
    offset += network->neuron_counts[i];
  }
  return offset;
}

static void calculate_output_layer(neural_network_t *network, size_t output_layer) {
  size_t in_size = network->neuron_counts[output_layer - 1];
  size_t out_size = network->neuron_counts[output_layer];
  size_t in_offset = neural_layer_offset(network, output_layer - 1);
  size_t out_offset = in_offset + in_size;

  const double *input = network->output + in_offset;
  double *output = network->output + out_offset;
  const double (*t_weights)[in_size] = neural_layer_t_weights(network, output_layer);
  const double *bias = network->bias + out_offset;

  for (size_t i = 0; i < out_size; i++) {
    double sum = bias[i];
    for (size_t j = 0; j < in_size; j++) {
      sum += t_weights[i][j] * input[j];
    }
    output[i] = neural_sigmoid(sum);
  }
}

/**
 * @brief Forward propagate A[layer], return next layer A[layer + 1]
 *
 * @param network The neural network to propagate through
 * @param layer The index of the layer to propagate from
 * @param m The number of training examples
 * @param A_in The input array for the current layer
 * @param A_out The output array for the next layer
 */
static void forward_propagate(neural_network_t *network, size_t layer, size_t m, const double *A_in, double *A_out) {
  size_t in_size = network->neuron_counts[layer];
  size_t out_size = network->neuron_counts[layer + 1];
  size_t out_offset = neural_layer_offset(network, layer + 1);

  const double (*W)[in_size] = neural_layer_t_weights(network, layer + 1);
  const double *b = network->bias + out_offset;

  // A[L+1] = sigmoid(Z[L+1] = W[L+1] * A[L] + b[L+1])
  for (size_t i = 0; i < out_size; ++i) {
    double *A_out_i = A_out + i * m;
    for (size_t j = 0; j < m; ++j) {
      A_out_i[j] = b[i];
    }

    // Cache optimized matrix multiplication
    for (size_t k = 0; k < in_size; ++k) {
      const double W_ik = W[i][k];
      const double *A_in_k = A_in + k * m;
      for (size_t j = 0; j < m; ++j) {
        A_out_i[j] += W_ik * A_in_k[j];
      }
    }

    for (size_t j = 0; j < m; ++j) {
      A_out_i[j] = neural_sigmoid(A_out_i[j]);
    }
  }
}