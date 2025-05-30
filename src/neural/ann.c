#include "neural/ann.h"

#include <math.h>
#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/util.h"

double neural_sigmoid(double x) { return 1.0 / (1.0 + exp(-x)); }

neural_network_t *create_neural_network(int num_hidden_layers, int neuron_counts_array[]) {
  num_hidden_layers = MAX(0, num_hidden_layers);
  neural_network_t *network = malloc(sizeof(neural_network_t));
  if (!network) {
    return NULL;
  }

  network->num_hidden_layers = num_hidden_layers;
  network->neuron_counts = NULL;
  network->output = NULL;
  network->weights = NULL;

  network->neuron_counts = calloc(num_hidden_layers + 2, sizeof(int));
  if (!network->neuron_counts) {
    free_neural_network(network);
    return NULL;
  }

  memcpy(network->neuron_counts, neuron_counts_array, (num_hidden_layers + 2) * sizeof(int));

  size_t output_size = 0;
  size_t weights_size = 0;
  for (int i = 0; i < num_hidden_layers + 1; i++) {
    const int in_size = network->neuron_counts[i];
    const int out_size = network->neuron_counts[i + 1];
    if (in_size <= 0 || out_size <= 0) {
      free_neural_network(network);
      return NULL;
    }

    output_size += out_size;
    weights_size += in_size * out_size;
  }

  network->output = calloc(output_size, sizeof(double));
  if (!network->output) {
    free_neural_network(network);
    return NULL;
  }

  network->weights = calloc(weights_size, sizeof(double));
  if (!network->weights) {
    free_neural_network(network);
    return NULL;
  }

  return network;
}

void randomize_weights(neural_network_t *network, double min_weight, double max_weight) {
  if (!network || !network->weights) {
    return;
  }

  for (int i = 0; i < network->num_hidden_layers + 1; i++) {
    int in_size = network->neuron_counts[i];
    int out_size = network->neuron_counts[i + 1];
    double (*layer_weights)[out_size] = neural_layer_weights(network, i);

    for (int j = 0; j < in_size; j++) {
      for (int k = 0; k < out_size; k++) {
        layer_weights[j][k] = min_weight + (max_weight - min_weight) * ((double)rand() / RAND_MAX);
      }
    }
  }
}

double (*neural_layer_weights(neural_network_t *network, int layer))[] {
  int out = network->neuron_counts[layer + 1];
  size_t offset = 0;
  for (int i = 0; i < layer; ++i) {
    int in_size = network->neuron_counts[i];
    int out_size = network->neuron_counts[i + 1];
    offset += in_size * out_size;
  }

  return (double (*)[out])(network->weights + offset);
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

  printf("\nWeights:");
  for (int i = 0; i < network->num_hidden_layers + 1; i++) {
    printf("\nLayer %d-%d:\n", i, i + 1);
    double (*layer_weights)[network->neuron_counts[i + 1]] = neural_layer_weights(network, i);
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
  if (network->weights) {
    free(network->weights);
  }
  free(network);
}
