#include "neural/nn.h"

#include <math.h>
#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/util.h"

static void calculate_output_layer(neural_network_t *network, int layer);

double neural_sigmoid(double x) { return 1.0 / (1.0 + exp(-x)); }

neural_network_t *create_neural_network(int num_hidden_layers, int neuron_counts_array[]) {
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

  int weights_size = 0;
  int total_neurons = neuron_counts_array[0];
  for (int i = 0; i < num_hidden_layers + 1; i++) {
    const int in_size = neuron_counts_array[i];
    const int out_size = neuron_counts_array[i + 1];
    if (in_size <= 0 || out_size <= 0) {
      free_neural_network(network);
      return NULL;
    }

    total_neurons += out_size;
    weights_size += in_size * out_size;
  }
  network->total_neurons = total_neurons;
  network->total_weights = weights_size;

  network->neuron_counts = calloc(num_hidden_layers + 2, sizeof(int));
  if (!network->neuron_counts) {
    free_neural_network(network);
    return NULL;
  }
  memcpy(network->neuron_counts, neuron_counts_array, (num_hidden_layers + 2) * sizeof(int));

  network->output = calloc(total_neurons - neuron_counts_array[0], sizeof(double));
  if (!network->output) {
    free_neural_network(network);
    return NULL;
  }

  network->t_weights = calloc(weights_size, sizeof(double));
  if (!network->t_weights) {
    free_neural_network(network);
    return NULL;
  }

  network->bias = calloc(total_neurons, sizeof(double));
  if (!network->bias) {
    free_neural_network(network);
    return NULL;
  }

  return network;
}

void randomize_weights(neural_network_t *network, double min_weight, double max_weight) {
  if (!network || !network->t_weights) {
    return;
  }

  for (int i = 0; i < network->total_weights; i++) {
    network->t_weights[i] = min_weight + (max_weight - min_weight) * ((double)rand() / RAND_MAX);
  }
}

void randomize_bias(neural_network_t *network, double min_bias, double max_bias) {
  if (!network || !network->bias) {
    return;
  }

  for (int i = 0; i < network->total_neurons; i++) {
    network->bias[i] = min_bias + (max_bias - min_bias) * ((double)rand() / RAND_MAX);
  }
}

double (*neural_layer_t_weights(neural_network_t *network, int out_layer))[] {
  int in_size = network->neuron_counts[out_layer - 1];
  size_t offset = 0;
  for (int i = 0; i < out_layer - 1; i++) {
    int out_size_prev = network->neuron_counts[i + 1];
    int in_size_prev = network->neuron_counts[i];

    offset += out_size_prev * in_size_prev;
  }
  return (double (*)[in_size])(network->t_weights + offset);
}

double *neural_layer_bias(neural_network_t *network, int layer) {
  size_t offset = 0;
  for (int i = 0; i < layer; i++) {
    offset += network->neuron_counts[i];
  }

  return network->bias + offset;
}

void print_neural_network(neural_network_t *network) {
  if (!network) {
    return;
  }

  printf("Neural Network Structure:\n");
  printf("Inputs: %d\n", network->neuron_counts[0]);
  printf("Hidden Layers: %d\n", network->num_hidden_layers);
  for (int i = 0; i < network->num_hidden_layers; i++) {
    printf("Hidden Layer %d: %d neurons\n", i + 1, network->neuron_counts[i + 1]);
  }
  printf("Outputs: %d\n", network->neuron_counts[network->num_hidden_layers + 1]);

  printf("\nT_Weights:");
  for (int i = 0; i < network->num_hidden_layers + 1; i++) {
    printf("\nLayer %d-%d:\n", i, i + 1);
    double (*layer_weights)[network->neuron_counts[i]] = neural_layer_t_weights(network, i + 1);
    for (int j = 0; j < network->neuron_counts[i]; j++) {
      printf("[");
      for (int k = 0; k < network->neuron_counts[i + 1]; k++) {
        if (k > 0) {
          printf(", ");
        }
        printf("%.3f", layer_weights[j][k]);
      }
      printf("]\n");
    }
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
  free(network);
}

static void calculate_output_layer(neural_network_t *network, int output_layer) {
  int in_size = network->neuron_counts[output_layer - 1];
  int out_size = network->neuron_counts[output_layer];
  double *input = network->output + (output_layer - 1 == 0 ? 0 : network->neuron_counts[output_layer - 2]);
  double *output = network->output + network->neuron_counts[output_layer - 1];
  double (*weights)[in_size] = neural_layer_t_weights(network, output_layer);
  double *bias = neural_layer_bias(network, output_layer);

  for (int i = 0; i < out_size; i++) {
    double sum = bias[i];
    for (int j = 0; j < in_size; j++) {
      sum += weights[i][j] * input[j];
    }
    output[i] = neural_sigmoid(sum);
  }
}