#include "neural/nn.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
  // Example usage of the neural network functions
  size_t neuron_counts[] = {
      10, 20, 40, 20, 5, 8};  // Input layer with 3 neurons, hidden layer with 5 neurons, output layer with 2 neurons
  neural_network_t *network = create_neural_network(4, neuron_counts);

  assert(network != NULL);
  assert(network->num_hidden_layers == 4);
  assert(network->total_neurons == 103);
  assert(network->total_weights == 1940);
  assert(network->neuron_counts != NULL);
  assert(network->output != NULL);
  assert(network->t_weights != NULL);
  assert(network->bias != NULL);

  // Randomize weights and biases
  srand(time(NULL));  // Seed the random number generator
  randomize_weights(network, -1.0, 1.0);
  randomize_bias(network, -0.5, 0.5);

  double input[] = {0.5, 0.2, 0.1};
  const double *output = run_neural_network(network, input);
  for (size_t i = 0; i < network->neuron_counts[network->num_hidden_layers + 1]; i++) {
    if (i > 0 && i < network->neuron_counts[network->num_hidden_layers + 1]) {
      printf(", ");
    }
    printf("%f", output[i]);
  }
  printf("]\n");

  return 0;
}