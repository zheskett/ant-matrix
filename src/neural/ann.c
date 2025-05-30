#include "neural/ann.h"

#include <math.h>
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

#include "util/util.h"

double neural_sigmoid(double x) { return 1.0 / (1.0 + exp(-x)); }

neural_network_t *create_neural_network(int num_inputs, int num_hidden_layers, int num_hidden_array[],
                                        int num_outputs) {
  num_hidden_layers = MAX(0, num_hidden_layers);
  neural_network_t *network = malloc(sizeof(neural_network_t));
  if (!network) {
    return NULL;
  }

  network->num_inputs = num_inputs;
  network->num_hidden_layers = num_hidden_layers;
  network->num_outputs = num_outputs;
  network->num_hidden = NULL;
  network->output = NULL;
  network->weights = NULL;

  if (num_hidden_layers == 0) {
    network->num_hidden = NULL;
  } else {
    network->num_hidden = calloc(num_hidden_layers, sizeof(int));
    if (!network->num_hidden) {
      free_neural_network(network);
      return NULL;
    }

    memcpy(network->num_hidden, num_hidden_array, num_hidden_layers * sizeof(int));
  }

  network->output = (double **)calloc(num_hidden_layers + 1, sizeof(double *));
  if (!network->output) {
    free_neural_network(network);
    return NULL;
  }

  for (int i = 0; i < num_hidden_layers + 1; i++) {
    const int out_size = i == num_hidden_layers ? num_outputs : num_hidden_array[i];
    network->output[i] = calloc(out_size, sizeof(double));
    if (!network->output[i]) {
      free_neural_network(network);
      return NULL;
    }
  }

  network->weights = (double ***)calloc(num_hidden_layers + 1, sizeof(double **));
  if (!network->weights) {
    free_neural_network(network);
    return NULL;
  }

  for (int i = 0; i < num_hidden_layers + 1; i++) {
    const int in_size = i == 0 ? num_inputs : num_hidden_array[i - 1];
    const int out_size = i == num_hidden_layers ? num_outputs : num_hidden_array[i];

    // Account for alignment and padding
    size_t row_bytes = in_size * sizeof(double *);
    size_t align = alignof(double);
    size_t pad = (align - (row_bytes % align)) % align;
    size_t total = row_bytes + pad + in_size * out_size * sizeof(double);

    char *base = calloc(total, 1);  // Allocate and zero-initialize memory
    if (!base) {
      free_neural_network(network);
      return NULL;
    }
    double **rows = (double **)base;
    double *block = (double *)(base + row_bytes + pad);

    network->weights[i] = rows;
    for (int j = 0; j < in_size; j++) {
      rows[j] = block + j * out_size;
    }
  }

  return network;
}

void randomize_weights(neural_network_t *network, double min_weight, double max_weight) {
  if (!network || !network->weights) {
    return;
  }

  for (int i = 0; i < network->num_hidden_layers + 1; i++) {
    for (int j = 0; j < (i == 0 ? network->num_inputs : network->num_hidden[i - 1]); j++) {
      for (int k = 0; k < (i == network->num_hidden_layers ? network->num_outputs : network->num_hidden[i]); k++) {
        network->weights[i][j][k] = min_weight + (rand() / (double)RAND_MAX) * (max_weight - min_weight);
      }
    }
  }
}

void print_neural_network(neural_network_t *network) {
  if (!network) {
    return;
  }

  printf("Neural Network Structure:\n");
  printf("Inputs: %d\n", network->num_inputs);
  printf("Hidden Layers: %d\n", network->num_hidden_layers);
  for (int i = 0; i < network->num_hidden_layers; i++) {
    printf("Hidden Layer %d: %d neurons\n", i + 1, network->num_hidden[i]);
  }
  printf("Outputs: %d\n", network->num_outputs);

  printf("\nWeights:");
  for (int i = 0; i < network->num_hidden_layers + 1; i++) {
    printf("\nLayer %d-%d:\n", i, i + 1);
    for (int j = 0; j < (i == 0 ? network->num_inputs : network->num_hidden[i - 1]); j++) {
      printf("[");
      for (int k = 0; k < (i == network->num_hidden_layers ? network->num_outputs : network->num_hidden[i]); k++) {
        if (k > 0) {
          printf(", ");
        }
        printf("%.3f", i, j, k, network->weights[i][j][k]);
      }
      printf("]\n");
    }
  }
}

void free_neural_network(neural_network_t *network) {
  if (!network) {
    return;
  }

  if (network->num_hidden) {
    free(network->num_hidden);
  }

  if (network->output) {
    for (int i = 0; i < network->num_hidden_layers + 1; i++) {
      if (network->output[i]) {
        free(network->output[i]);
      }
    }
    free(network->output);
  }

  if (network->weights) {
    for (int i = 0; i < network->num_hidden_layers + 1; i++) {
      if (network->weights[i]) {
        free(network->weights[i]);
      }
    }
    free(network->weights);
  }

  free(network);
}