#include "neural/nn.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
  size_t neuron_counts[] = {3, 20, 4, 3, 4, 5};
  neural_network_t *network = create_neural_network((sizeof(neuron_counts) / sizeof(size_t)) - 2, neuron_counts);

  assert(network != NULL);
  assert(network->num_hidden_layers == (sizeof(neuron_counts) / sizeof(size_t)) - 2);
  assert(network->neuron_counts != NULL);
  assert(network->output != NULL);
  assert(network->t_weights != NULL);
  assert(network->bias != NULL);

  srand(time(NULL));
  randomize_weights(network, -1.0, 1.0);
  randomize_bias(network, -0.5, 0.5);

  double input[] = {0.5, 0.2, 0.1};
  const double *output = run_neural_network(network, input);
  printf("[");
  for (size_t i = 0; i < network->neuron_counts[network->num_hidden_layers + 1]; i++) {
    if (i > 0 && i < network->neuron_counts[network->num_hidden_layers + 1]) {
      printf(", ");
    }
    printf("%f", output[i]);
  }
  printf("]\n");
  write_neural_network(network, stdout);

  double score = train_neural_network(network, 1, input, output, 0.01);
  assert(score < 1e-6 && score > -1e-6);

  score = train_neural_network(network, 1, input, (const double[5]){0.5, 0.5, 0.5, 0.5, 0.5}, 0.01);
  printf("Cost after training: %f\n", score);

  free_neural_network(network);
  return 0;
}