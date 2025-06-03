#include "neural/nn.h"

#include <stdlib.h>
#include <string.h>

#include "util/util.h"

static void calculate_output_layer(neural_network_t *network, size_t output_layer);
static size_t neural_layer_offset(neural_network_t *network, size_t layer);
static char *allocate_data(neural_network_t *network, size_t m);

static inline double neural_sigmoid(double x) { return (x > 45 ? 1.0 : (x < -45 ? -1.0 : 1.0 / (1.0 + exp(-x)))); }

neural_network_t *create_neural_network(size_t num_hidden_layers, size_t neuron_counts_array[]) {
  num_hidden_layers = MAX(0, num_hidden_layers);
  neural_network_t *network = malloc(sizeof(neural_network_t));
  if (!network) {
    return NULL;
  }

  network->num_hidden_layers = num_hidden_layers;
  network->total_neurons = 0;
  network->total_weights = 0;
  network->data_size = 0;
  network->data = NULL;
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

  // Calculate the size of the data used for training and inference (8 is arbitrary)
  allocate_data(network, 8);
  if (!network->data) {
    free_neural_network(network);
    return NULL;
  }

  return network;
}

const double *run_neural_network(neural_network_t *network, const double *input) {
  memcpy(network->output, input, network->neuron_counts[0] * sizeof(double));
  for (size_t i = 1; i < network->num_hidden_layers + 2; i++) {
    calculate_output_layer(network, i);
  }

  return (const double *)(network->output + neural_layer_offset(network, network->num_hidden_layers + 1));
}

double train_neural_network(neural_network_t *network, size_t m, const double *inputs, const double *desired_outputs,
                            double lr) {
  if (!network || !inputs || !desired_outputs || m == 0) {
    return NAN;
  }
  const double m_inv = 1.0 / (double)m;
  const size_t num_layers = network->num_hidden_layers + 2;
  // Calculate required memory
  const size_t Y_items = network->neuron_counts[num_layers - 1] * m;
  const size_t A_matrices_items = network->total_neurons * m;
  const size_t delta_matrices_items = network->total_neurons * m;
  const size_t dC_dW_items = network->total_weights;
  const size_t L = num_layers - 1;

  double cost = 0.0;
  double *data_ptr = (double *)allocate_data(network, m);
  double *Y = NULL;
  double *A[num_layers];
  double *delta[num_layers];
  double *dC_dW[num_layers];

  if (!data_ptr) {
    return NAN;
  }

  // Allocate memory for A, delta, dC_db, and dC_dW
  Y = data_ptr;
  data_ptr += Y_items;
  for (size_t i = 0; i < num_layers; i++) {
    A[i] = data_ptr;
    data_ptr += network->neuron_counts[i] * m;
  }
  for (size_t i = 0; i < num_layers; i++) {
    delta[i] = data_ptr;
    data_ptr += network->neuron_counts[i] * m;
  }
  for (size_t i = 0; i < num_layers - 1; i++) {
    dC_dW[i] = data_ptr;
    data_ptr += network->neuron_counts[i + 1] * network->neuron_counts[i];
  }

  // Transpose inputs, desired_outputs for Y and A[0]
  for (size_t j = 0; j < m; j++) {
    const double *do_j = desired_outputs + j * network->neuron_counts[L];
    const double *in_j = inputs + j * network->neuron_counts[0];
    for (size_t i = 0; i < network->neuron_counts[L]; i++) {
      Y[i * m + j] = do_j[i];
    }

    for (size_t i = 0; i < network->neuron_counts[0]; i++) {
      A[0][i * m + j] = in_j[i];
    }
  }

  // Feed forward through the network
  for (size_t i = 0; i < L; i++) {
    forward_propagate_layer(network, i, m, A[i], A[i + 1]);
  }

  cost = calculate_cost(network, m, Y, A[L]);

  // Backpropagation and weight updates
  /*
  (delta^L) = dC/dZ^L = dC/d(Y_hat) * d(Y_hat)/dZ^L = (1/m)(Y_hat - Y) * d(Y_hat)/dZ^L
  dC/d(Y_hat) = (1/m) * (Y_hat - Y)
  d(Y_hat)/dZ^L = Y_hat * (1 - Y_hat) (sigmoid derivative)
  d(Y_hat)/dZ^L = 1 - Y_hat^2 (tanh derivative)

  dZ_{j,i}/db_{j} = 1
  dC/db^L = delta * dZ/db = delta^L x 1(m x 1) = ∑ delta^L_{:,i}
  dC/dW^L = (delta^L)((A^{L-1})^T)

  dC/dA^[l] = dZ^[l+1]/dA^[l] * dC/dZ^[l+1] = (W^[l+1]^T)(delta^[l+1])
  delta^[l] = dC/dA^[l] * dA^[l]/dZ^[l] = (W^[l+1]^T)(delta^[l+1]) * (1 - A^[l]^2)
  dC/db^[l] = ∑(delta^[l]_{:,i})
  dC/dW^[l] = (delta^[l])(A^[l-1]^T)
  */
  // Output layer
  double *bias_L = network->bias + neural_layer_offset(network, L);
  for (size_t i = 0; i < network->neuron_counts[L]; i++) {
    double sum = 0.0;
    double *delta_Li = delta[L] + i * m;
    const double *A_Li = A[L] + i * m;
    const double *Y_i = Y + i * m;
    for (size_t j = 0; j < m; j++) {
      const double value = m_inv * (A_Li[j] - Y_i[j]) * (1 - A_Li[j] * A_Li[j]);
      delta_Li[j] = value;
      sum += value;
    }
    // Gradient descent bias
    bias_L[i] -= lr * sum;
  }

  // dC/dW^L = (delta^L)((A^{L-1})^T)
  for (size_t i = 0; i < network->neuron_counts[L]; i++) {
    // L-1 since 0-indexed
    double *dC_dW_Li = dC_dW[L - 1] + i * network->neuron_counts[L - 1];
    const double *delta_Li = delta[L] + i * m;
    for (size_t j = 0; j < network->neuron_counts[L - 1]; j++) {
      const double *A_L1j = A[L - 1] + j * m;
      double sum = 0.0;
      for (size_t k = 0; k < m; k++) {
        sum += delta_Li[k] * A_L1j[k];
      }
      // Gradient descent weight, cannot adjust the in-place weights yet
      dC_dW_Li[j] = sum;
    }
  }

  // Hidden layers (1 - L-1)
  for (size_t l = num_layers - 2; l > 0; l--) {
    // dC/dA^[l] = (W^[l+1]^T)(delta^[l+1])
    const size_t in_size = network->neuron_counts[l];
    const size_t out_size = network->neuron_counts[l + 1];
    const double (*W_l1)[in_size] = neural_layer_t_weights(network, l + 1);
    const double *delta_l1 = delta[l + 1];
    double *bias_l = network->bias + neural_layer_offset(network, l);

    // Cache optimized order
    for (size_t j = 0; j < in_size; j++) {
      double sum = 0.0;
      const double *A_lj = A[l] + j * m;
      double *delta_lj = delta[l] + j * m;
      for (size_t i = 0; i < out_size; i++) {
        const double *delta_l1i = delta[l + 1] + i * m;
        for (size_t k = 0; k < m; k++) {
          // 0-initialized because of calloc
          delta_lj[k] += W_l1[i][j] * delta_l1i[k];
        }
      }

      // delta^l = dC/dA^[l] * dA^[l]/dZ^[l]
      for (size_t k = 0; k < m; k++) {
        delta_lj[k] *= (1 - A_lj[k] * A_lj[k]);
        sum += delta_lj[k];
      }

      // Gradient descent bias
      bias_l[j] -= lr * sum;
    }

    // dC/dW^l = (delta^l)((A^{l-1})^T)
    for (size_t i = 0; i < in_size; i++) {
      // l-1 since 0-indexed
      double *dC_dW_li = dC_dW[l - 1] + i * network->neuron_counts[l - 1];
      const double *delta_li = delta[l] + i * m;
      for (size_t j = 0; j < network->neuron_counts[l - 1]; j++) {
        const double *A_l1j = A[l - 1] + j * m;
        double sum = 0.0;
        for (size_t k = 0; k < m; k++) {
          sum += delta_li[k] * A_l1j[k];
        }
        // Gradient descent weight, cannot adjust the in-place weights yet
        dC_dW_li[j] = sum;
      }
    }
  }

  // Apply gradient descent on weights
  for (size_t i = 0; i < network->total_weights; i++) {
    network->t_weights[i] -= lr * (*dC_dW)[i];
  }

  return cost;
}

double calculate_cost(neural_network_t *network, size_t m, const double *y, const double *y_hat) {
  if (!network || !y || !y_hat || m == 0) {
    return NAN;
  }

  double sum = 0.0;
  const double mx2_inv = 1.0 / (2.0 * (double)m);
  const size_t total = network->neuron_counts[network->num_hidden_layers + 1] * m;
  for (size_t i = 0; i < total; ++i) {
    const double diff = y[i] - y_hat[i];
    sum += diff * diff * mx2_inv;
  }
  return sum;
}

void forward_propagate_layer(neural_network_t *network, size_t in_layer, size_t m, const double *A_in, double *A_out) {
  size_t in_size = network->neuron_counts[in_layer];
  size_t out_size = network->neuron_counts[in_layer + 1];
  size_t out_offset = neural_layer_offset(network, in_layer + 1);

  const double (*W)[in_size] = neural_layer_t_weights(network, in_layer + 1);
  const double *b = network->bias + out_offset;

  // A[L+1] = tanh(Z[L+1] = W[L+1] * A[L] + b[L+1])
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
      A_out_i[j] = tanh(A_out_i[j]);
    }
  }
}

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
  fprintf(fp, "Hidden Layers: %zu\n", network->num_hidden_layers);
  for (size_t i = 0; i < network->num_hidden_layers; i++) {
    fprintf(fp, "Hidden Layer %zu: %zu neurons\n", i + 1, network->neuron_counts[i + 1]);
  }
  fprintf(fp, "Outputs: %zu\n", network->neuron_counts[network->num_hidden_layers + 1]);

  fprintf(fp, "\nWeights:");
  for (size_t i = 0; i < network->num_hidden_layers + 1; i++) {
    fprintf(fp, "\nLayer %zu-%zu:\n", i, i + 1);
    double (*layer_weights)[network->neuron_counts[i]] = neural_layer_t_weights(network, i + 1);
    for (size_t j = 0; j < network->neuron_counts[i]; j++) {
      fprintf(fp, "[");
      for (size_t k = 0; k < network->neuron_counts[i + 1]; k++) {
        if (k > 0) {
          fprintf(fp, ", ");
        }
        fprintf(fp, "% .2f", layer_weights[k][j]);
      }
      fprintf(fp, "]\n");
    }
  }

  fprintf(fp, "\nBiases:\n");
  for (size_t i = 1; i < network->num_hidden_layers + 2; i++) {
    fprintf(fp, "Layer %zu: ", i);
    double *bias = network->bias + neural_layer_offset(network, i);
    for (size_t j = 0; j < network->neuron_counts[i]; j++) {
      if (j > 0) {
        fprintf(fp, ", ");
      }
      fprintf(fp, "% .2f", bias[j]);
    }
    fprintf(fp, "\n");
  }

  fprintf(fp, "\nOutputs:\n");
  for (size_t i = 0; i < network->num_hidden_layers + 2; i++) {
    fprintf(fp, "Layer %zu: ", i);
    double *output = network->output + neural_layer_offset(network, i);
    for (size_t j = 0; j < network->neuron_counts[i]; j++) {
      if (j > 0) {
        fprintf(fp, ", ");
      }
      fprintf(fp, "% .2f", output[j]);
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
  if (network->data) {
    free(network->data);
  }
  network->data = NULL;
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
    output[i] = tanh(sum);
  }
}

/**
 * @brief Allocate memory for the data used in training and inference.
 *
 * @param network The neural network for which to allocate data.
 * @param m The number of training examples.
 * @return A pointer to the allocated data, or NULL if allocation fails.
 */
static char *allocate_data(neural_network_t *network, size_t m) {
  if (!network || m == 0) {
    return NULL;
  }

  const size_t num_layers = network->num_hidden_layers + 2;
  const size_t Y_items = network->neuron_counts[num_layers - 1] * m;
  const size_t A_matrices_items = network->total_neurons * m;
  const size_t delta_matrices_items = network->total_neurons * m;
  const size_t dC_dW_items = network->total_weights;
  const size_t size = (Y_items + A_matrices_items + delta_matrices_items + dC_dW_items) * sizeof(double);
  if (size > network->data_size) {
    if (network->data) {
      free(network->data);
    }
    network->data_size = size;
    network->data = malloc(size);
  }

  return network->data;
}