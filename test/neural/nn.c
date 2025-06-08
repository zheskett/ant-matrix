#include "neural/nn.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int test_xor() {
  int neuron_counts[] = {2, 2, 1};
  neural_network_t *network = neural_create(1, neuron_counts);
  assert(network != NULL);

  double std = sqrt(6) / sqrt(network->neuron_counts[0] + network->neuron_counts[2]);
  neural_randomize_weights(network, -std, std);
  neural_randomize_bias(network, 0, 0);

  const double inputs[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
  const double expected_outputs[4][1] = {{-1}, {1}, {1}, {-1}};

  for (int i = 0; i < 1000; i++) {
    // Randomly select 2 inputs and their expected output
    int rand_index = rand() % 4;
    int prev_index = rand_index;
    while (rand_index == prev_index) {
      rand_index = rand() % 4;
    }

    const double inputs_ptr[4] = {inputs[rand_index][0], inputs[rand_index][1], inputs[prev_index][0],
                                  inputs[prev_index][1]};
    const double outputs_ptr[2] = {expected_outputs[rand_index][0], expected_outputs[prev_index][0]};

    double error = neural_train(network, 2, inputs_ptr, outputs_ptr, 0.05);
    // printf("%f\n", error);
  }

  neural_run(network, inputs[1]);
  neural_print(network, stdout);

  for (int i = 0; i < 4; i++) {
    const double *output = neural_run(network, inputs[i]);
    printf("Input: [%f, %f] -> Output: [%f]\n", inputs[i][0], inputs[i][1], output[0]);
    fflush(stdout);
    assert(fabs(output[0] - expected_outputs[i][0]) < 0.2);
  }

  neural_free(network);
  return EXIT_SUCCESS;
}

int test_read_write() {
  int neuron_counts[] = {3, 20, 4, 3, 4, 5};
  neural_network_t *network = neural_create((sizeof(neuron_counts) / sizeof(int)) - 2, neuron_counts);
  assert(network != NULL);

  neural_randomize_weights(network, -1.0, 1.0);
  neural_randomize_bias(network, -0.5, 0.5);

  if (!neural_write(network, "neural_write_test.bin")) {
    return EXIT_FAILURE;
  }

  neural_network_t *read_network = neural_read("neural_write_test.bin");
  if (!read_network) {
    return EXIT_FAILURE;
  }

  assert(read_network->num_hidden_layers == network->num_hidden_layers);
  assert(read_network->total_neurons == network->total_neurons);
  assert(read_network->total_weights == network->total_weights);
  assert(read_network->neuron_counts != NULL);
  for (int i = 0; i < read_network->num_hidden_layers + 2; i++) {
    assert(read_network->neuron_counts[i] == network->neuron_counts[i]);
  }
  assert(read_network->output != NULL);
  assert(read_network->t_weights != NULL);
  for (int i = 0; i < read_network->total_weights; i++) {
    assert(read_network->t_weights[i] == network->t_weights[i]);
  }
  assert(read_network->bias != NULL);
  for (int i = 0; i < read_network->total_neurons; i++) {
    assert(read_network->bias[i] == network->bias[i]);
  }
  assert(read_network->data != NULL);
  assert(read_network->data_size == INITIAL_DATA_SIZE * sizeof(double));

  neural_free(read_network);
  neural_free(network);
  return EXIT_SUCCESS;
}

int main() {
  int neuron_counts[] = {3, 20, 4, 3, 4, 5};
  neural_network_t *network = neural_create((sizeof(neuron_counts) / sizeof(int)) - 2, neuron_counts);

  assert(network != NULL);
  assert(network->num_hidden_layers == (sizeof(neuron_counts) / sizeof(int)) - 2);
  assert(network->neuron_counts != NULL);
  assert(network->output != NULL);
  assert(network->t_weights != NULL);
  assert(network->bias != NULL);

  srand(time(NULL));
  neural_randomize_weights(network, -1.0, 1.0);
  neural_randomize_bias(network, -0.5, 0.5);

  double input[] = {0.5, 0.2, 0.1};
  const double *output = neural_run(network, input);
  // printf("[");
  // for (int i = 0; i < network->neuron_counts[network->num_hidden_layers + 1]; i++) {
  //   if (i > 0 && i < network->neuron_counts[network->num_hidden_layers + 1]) {
  //     printf(", ");
  //   }
  //   printf("%f", output[i]);
  // }
  // printf("]\n");
  // neural_print(network, stdout);

  double score = neural_train(network, 1, input, output, 0.01);
  assert(score < 1e-6 && score > -1e-6);

  score = neural_train(network, 1, input, (const double[5]){0.5, 0.5, 0.5, 0.5, 0.5}, 0.01);
  printf("Cost after training: %f\n", score);

  neural_free(network);
  fflush(stdout);
  if (test_xor() != EXIT_SUCCESS || test_read_write() != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}