#include "neural/nn.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int test_xor() {
  size_t neuron_counts[] = {2, 2, 1};
  neural_network_t *network = create_neural_network(1, neuron_counts);
  assert(network != NULL);

  double std = sqrt(6) / sqrt(network->neuron_counts[0] + network->neuron_counts[2]);
  randomize_weights(network, -std, std);
  randomize_bias(network, 0, 0);

  const double inputs[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
  const double expected_outputs[4][1] = {{-1}, {1}, {1}, {-1}};

  for (size_t i = 0; i < 1000; i++) {
    // Randomly select 2 inputs and their expected output
    int rand_index = rand() % 4;
    int prev_index = rand_index;
    while (rand_index == prev_index) {
      rand_index = rand() % 4;
    }

    const double inputs_ptr[4] = {inputs[rand_index][0], inputs[rand_index][1], inputs[prev_index][0],
                                  inputs[prev_index][1]};
    const double outputs_ptr[2] = {expected_outputs[rand_index][0], expected_outputs[prev_index][0]};

    double error = train_neural_network(network, 2, inputs_ptr, outputs_ptr, 0.05);
    // printf("%f\n", error);
  }

  run_neural_network(network, inputs[1]);
  write_neural_network(network, stdout);

  for (size_t i = 0; i < 4; i++) {
    const double *output = run_neural_network(network, inputs[i]);
    printf("Input: [%f, %f] -> Output: [%f]\n", inputs[i][0], inputs[i][1], output[0]);
    fflush(stdout);
    assert(fabs(output[0] - expected_outputs[i][0]) < 0.2);
  }

  free_neural_network(network);
  return EXIT_SUCCESS;
}

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
  // write_neural_network(network, stdout);

  double score = train_neural_network(network, 1, input, output, 0.01);
  assert(score < 1e-6 && score > -1e-6);

  score = train_neural_network(network, 1, input, (const double[5]){0.5, 0.5, 0.5, 0.5, 0.5}, 0.01);
  printf("Cost after training: %f\n", score);

  free_neural_network(network);
  fflush(stdout);
  return test_xor();
}