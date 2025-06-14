#include "neural/nn.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "util/util.h"

static double calculate_cost(neural_network_t *network, int m, const double *y, const double *y_hat);
static void forward_propagate_layer(neural_network_t *network, int layer, int m, const double *A_in, double *A_out);
static void calculate_output_layer(neural_network_t *network, int output_layer);
static int neural_layer_offset(neural_network_t *network, int layer);
static char *allocate_data(neural_network_t *network, int m);

static inline double neural_sigmoid(double x) { return (x > 45 ? 1.0 : (x < -45 ? -1.0 : 1.0 / (1.0 + exp(-x)))); }
double enc(double x) { return 0.5 * (x + 1.0); }
double dec(double x) { return 2.0 * x - 1.0; }

neural_network_t *neural_create(int num_hidden_layers, const int neuron_counts_array[]) {
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

  int weights_size = 0;
  int total_neurons = neuron_counts_array[0];
  for (int i = 0; i < num_hidden_layers + 1; i++) {
    const int in_size = neuron_counts_array[i];
    const int out_size = neuron_counts_array[i + 1];
    if (in_size <= 0 || out_size <= 0) {
      neural_free(network);
      return NULL;
    }

    total_neurons += out_size;
    weights_size += in_size * out_size;
  }
  network->total_neurons = total_neurons;
  network->total_weights = weights_size;

  network->neuron_counts = calloc(num_hidden_layers + 2, sizeof(*network->neuron_counts));
  if (!network->neuron_counts) {
    neural_free(network);
    return NULL;
  }
  memcpy(network->neuron_counts, neuron_counts_array, (num_hidden_layers + 2) * sizeof(*network->neuron_counts));

  network->output = calloc(total_neurons, sizeof(*network->output));
  if (!network->output) {
    neural_free(network);
    return NULL;
  }

  network->t_weights = calloc(weights_size, sizeof(*network->t_weights));
  if (!network->t_weights) {
    neural_free(network);
    return NULL;
  }

  network->bias = calloc(total_neurons, sizeof(*network->bias));
  if (!network->bias) {
    neural_free(network);
    return NULL;
  }

  // Calculate the size of the data used for training and inference (8 is arbitrary)
  allocate_data(network, INITIAL_DATA_SIZE);
  if (!network->data) {
    neural_free(network);
    return NULL;
  }

  return network;
}

const double *neural_run(neural_network_t *network, const double *input) {
  memcpy(network->output, input, network->neuron_counts[0] * sizeof(double));
  for (int i = 1; i < network->num_hidden_layers + 2; i++) {
    calculate_output_layer(network, i);
  }

  return (const double *)(network->output + neural_layer_offset(network, network->num_hidden_layers + 1));
}

double neural_train(neural_network_t *network, int m, const double *inputs, const double *desired_outputs, double lr) {
  if (!network || !inputs || !desired_outputs || m == 0) {
    return NAN;
  }
  const double m_inv = 1.0 / (double)m;
  const int num_layers = network->num_hidden_layers + 2;
  // Calculate required memory
  const int Y_items = network->neuron_counts[num_layers - 1] * m;
  const int L = num_layers - 1;

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
  for (int i = 0; i < num_layers; i++) {
    A[i] = data_ptr;
    data_ptr += network->neuron_counts[i] * m;
  }
  for (int i = 0; i < num_layers; i++) {
    delta[i] = data_ptr;
    data_ptr += network->neuron_counts[i] * m;
  }
  for (int i = 0; i < num_layers - 1; i++) {
    dC_dW[i] = data_ptr;
    data_ptr += network->neuron_counts[i + 1] * network->neuron_counts[i];
  }

  // Transpose inputs, desired_outputs for Y and A[0]
  // #pragma omp parallel for
  for (int j = 0; j < m; j++) {
    const double *do_j = desired_outputs + j * network->neuron_counts[L];
    const double *in_j = inputs + j * network->neuron_counts[0];
    for (int i = 0; i < network->neuron_counts[L]; i++) {
      Y[i * m + j] = do_j[i];
    }

    for (int i = 0; i < network->neuron_counts[0]; i++) {
      A[0][i * m + j] = in_j[i];
    }
  }

  memset(delta[0], 0, network->total_neurons * m * sizeof(double));

  // Feed forward through the network
  for (int i = 0; i < L; i++) {
    forward_propagate_layer(network, i, m, A[i], A[i + 1]);
  }

  cost = calculate_cost(network, m, Y, A[L]);

  // Backpropagation and weight updates
  /*
  (delta^L) = dC/dZ^L = dC/d(Y_hat) * d(Y_hat)/dZ^L = (1/m)(Y_hat - Y) * d(Y_hat)/dZ^L
  dC/d(Y_hat) = (1/m) * (Y_hat - Y) (mean squared error)
  dC/d(Z^L) (delta^L) = (1/m) * (Y_hat - Y) (binary cross-entropy)
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
  for (int i = 0; i < network->neuron_counts[L]; i++) {
    double sum = 0.0;
    double *delta_Li = delta[L] + i * m;
    const double *A_Li = A[L] + i * m;
    const double *Y_i = Y + i * m;
    for (int k = 0; k < m; k++) {
      const double value = m_inv * (A_Li[k] - Y_i[k]);
      delta_Li[k] = value;
      sum += value;
    }
    // Gradient descent bias
    bias_L[i] -= lr * sum;
  }

  // dC/dW^L = (delta^L)((A^{L-1})^T)
  for (int i = 0; i < network->neuron_counts[L]; i++) {
    // L-1 since 0-indexed
    double *dC_dW_Li = dC_dW[L - 1] + i * network->neuron_counts[L - 1];
    const double *delta_Li = delta[L] + i * m;
    for (int j = 0; j < network->neuron_counts[L - 1]; j++) {
      const double *A_L1j = A[L - 1] + j * m;
      double sum = 0.0;
      for (int k = 0; k < m; k++) {
        sum += delta_Li[k] * A_L1j[k];
      }
      // Gradient descent weight, cannot adjust the in-place weights yet
      dC_dW_Li[j] = sum;
    }
  }

  // Hidden layers (1 - L-1)
  for (int l = num_layers - 2; l >= 1; l--) {
    // dC/dA^[l] = (W^[l+1]^T)(delta^[l+1])
    const int in_size = network->neuron_counts[l];
    const int out_size = network->neuron_counts[l + 1];
    const double (*W_l1)[in_size] = neural_layer_t_weights(network, l + 1);
    const double *delta_l1 = delta[l + 1];
    double *delta_l = delta[l];
    double *bias_l = network->bias + neural_layer_offset(network, l);

    // Cache optimized order
    // #pragma omp parallel for
    for (int j = 0; j < in_size; j++) {
      double *delta_lj = delta_l + j * m;
      double sum = 0.0;
      const double *A_lj = A[l] + j * m;
      for (int i = 0; i < out_size; i++) {
        const double *delta_l1i = delta_l1 + i * m;
        for (int k = 0; k < m; k++) {
          delta_lj[k] += W_l1[i][j] * delta_l1i[k];
        }
      }

      // delta^l = dC/dA^[l] * dA^[l]/dZ^[l]
      for (int k = 0; k < m; k++) {
        delta_lj[k] *= (A_lj[k] * (1 - A_lj[k]));
        sum += delta_lj[k];
      }

      // Gradient descent bias
      bias_l[j] -= lr * sum;
    }

    // dC/dW^l = (delta^l)((A^{l-1})^T)
    // #pragma omp parallel for
    for (int i = 0; i < in_size; i++) {
      // l-1 since 0-indexed
      double *dC_dW_li = dC_dW[l - 1] + i * network->neuron_counts[l - 1];
      const double *delta_li = delta_l + i * m;
      for (int j = 0; j < network->neuron_counts[l - 1]; j++) {
        const double *A_l1j = A[l - 1] + j * m;
        double sum = 0.0;
        for (int k = 0; k < m; k++) {
          sum += delta_li[k] * A_l1j[k];
        }
        // Gradient descent weight, cannot adjust the in-place weights yet
        dC_dW_li[j] = sum;
      }
    }
  }

  // Apply gradient descent on weights
  // #pragma omp parallel for
  for (int i = 0; i < network->total_weights; i++) {
    network->t_weights[i] -= lr * (*dC_dW)[i];
  }

  return cost;
}

void neural_randomize_weights(neural_network_t *network, double min_weight, double max_weight) {
  if (!network || !network->t_weights) {
    return;
  }

  for (int i = 0; i < network->total_weights; i++) {
    network->t_weights[i] = min_weight + (max_weight - min_weight) * ((double)rand() / RAND_MAX);
  }
}

void neural_randomize_bias(neural_network_t *network, double min_bias, double max_bias) {
  if (!network || !network->bias) {
    return;
  }

  for (int i = 0; i < network->total_neurons; i++) {
    network->bias[i] = min_bias + (max_bias - min_bias) * ((double)rand() / RAND_MAX);
  }
}

double (*neural_layer_t_weights(neural_network_t *network, int out_layer))[] {
  int in_size = network->neuron_counts[out_layer - 1];
  int offset = 0;
  for (int i = 0; i < out_layer - 1; i++) {
    int out_size_prev = network->neuron_counts[i + 1];
    int in_size_prev = network->neuron_counts[i];

    offset += out_size_prev * in_size_prev;
  }
  return (double (*)[in_size])(network->t_weights + offset);
}

void neural_print(neural_network_t *network, FILE *fp) {
  if (!network || !fp) {
    return;
  }

  fprintf(fp, "Neural Network Structure:\n");
  fprintf(fp, "Inputs: %d\n", network->neuron_counts[0]);
  fprintf(fp, "Hidden Layers: %d\n", network->num_hidden_layers);
  for (int i = 0; i < network->num_hidden_layers; i++) {
    fprintf(fp, "Hidden Layer %d: %d neurons\n", i + 1, network->neuron_counts[i + 1]);
  }
  fprintf(fp, "Outputs: %d\n", network->neuron_counts[network->num_hidden_layers + 1]);

  fprintf(fp, "\nWeights:");
  for (int i = 0; i < network->num_hidden_layers + 1; i++) {
    fprintf(fp, "\nLayer %d-%d:\n", i, i + 1);
    double (*layer_weights)[network->neuron_counts[i]] = neural_layer_t_weights(network, i + 1);
    for (int j = 0; j < network->neuron_counts[i]; j++) {
      fprintf(fp, "[");
      for (int k = 0; k < network->neuron_counts[i + 1]; k++) {
        if (k > 0) {
          fprintf(fp, ", ");
        }
        fprintf(fp, "% .2f", layer_weights[k][j]);
      }
      fprintf(fp, "]\n");
    }
  }

  fprintf(fp, "\nBiases:\n");
  for (int i = 1; i < network->num_hidden_layers + 2; i++) {
    fprintf(fp, "Layer %d: ", i);
    double *bias = network->bias + neural_layer_offset(network, i);
    for (int j = 0; j < network->neuron_counts[i]; j++) {
      if (j > 0) {
        fprintf(fp, ", ");
      }
      fprintf(fp, "% .2f", bias[j]);
    }
    fprintf(fp, "\n");
  }

  fprintf(fp, "\nOutputs:\n");
  for (int i = 0; i < network->num_hidden_layers + 2; i++) {
    fprintf(fp, "Layer %d: ", i);
    double *output = network->output + neural_layer_offset(network, i);
    for (int j = 0; j < network->neuron_counts[i]; j++) {
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

bool neural_write(neural_network_t *network, const char *filename) {
  if (!network || !filename) {
    return false;
  }

  FILE *fp = fopen(filename, "wb");
  if (!fp) {
    fprintf(stderr, "Could not open file %s", filename);
    return false;
  }

  int neural_structure[3] = {network->num_hidden_layers, network->total_neurons, network->total_weights};
  fwrite(neural_structure, sizeof(int), sizeof(neural_structure) / sizeof(int), fp);

  // Write neuron counts, t_weights, bias. Ignore output/data
  fwrite(network->neuron_counts, sizeof(int), network->num_hidden_layers + 2, fp);
  fwrite(network->t_weights, sizeof(double), network->total_weights, fp);
  fwrite(network->bias, sizeof(double), network->total_neurons, fp);

  if (ferror(fp)) {
    fclose(fp);
    fprintf(stderr, "Could not write to file %s", filename);
    return false;
  }

  fclose(fp);
  return true;
}

neural_network_t *neural_read(const char *filename) {
  if (!filename) {
    return NULL;
  }

  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    fprintf(stderr, "Could not open file %s", filename);
    return NULL;
  }

  int neural_structure[3];
  fread(neural_structure, sizeof(int), sizeof(neural_structure) / sizeof(int), fp);
  if (ferror(fp)) {
    fclose(fp);
    fprintf(stderr, "Could not read file %s", filename);
    return NULL;
  }

  int num_hidden_layers = neural_structure[0];
  int total_neurons = neural_structure[1];
  int total_weights = neural_structure[2];

  int *neuron_counts = malloc((num_hidden_layers + 2) * sizeof(int));
  if (!neuron_counts) {
    fclose(fp);
    return NULL;
  }
  fread(neuron_counts, sizeof(int), num_hidden_layers + 2, fp);
  if (ferror(fp)) {
    fclose(fp);
    fprintf(stderr, "Could not read file %s", filename);
    return NULL;
  }

  double *t_weights = malloc(total_weights * sizeof(double));
  if (!t_weights) {
    free(neuron_counts);
    fclose(fp);
    return NULL;
  }
  fread(t_weights, sizeof(double), total_weights, fp);
  if (ferror(fp)) {
    free(neuron_counts);
    free(t_weights);
    fclose(fp);
    fprintf(stderr, "Could not read file %s", filename);
    return NULL;
  }

  double *bias = malloc(total_neurons * sizeof(double));
  if (!bias) {
    free(neuron_counts);
    free(t_weights);
    fclose(fp);
    return NULL;
  }
  fread(bias, sizeof(double), total_neurons, fp);
  if (ferror(fp)) {
    free(neuron_counts);
    free(t_weights);
    free(bias);
    fclose(fp);
    fprintf(stderr, "Could not read file %s", filename);
    return NULL;
  }

  fclose(fp);

  neural_network_t *network = malloc(sizeof(neural_network_t));
  if (!network) {
    free(neuron_counts);
    free(t_weights);
    free(bias);
    return NULL;
  }

  network->num_hidden_layers = num_hidden_layers;
  network->total_neurons = total_neurons;
  network->total_weights = total_weights;
  network->neuron_counts = neuron_counts;
  network->output = calloc(total_neurons, sizeof(double));
  network->t_weights = t_weights;
  network->bias = bias;

  network->data = NULL;
  network->data_size = 0;
  allocate_data(network, INITIAL_DATA_SIZE);
  if (!network->data) {
    neural_free(network);
    return NULL;
  }

  return network;
}

neural_network_t *neural_copy(const neural_network_t *network) {
  if (!network) {
    return NULL;
  }

  neural_network_t *copy = neural_create(network->num_hidden_layers, network->neuron_counts);
  if (!copy) {
    return NULL;
  }

  // Copy weights, biases, and outputs
  memcpy(copy->t_weights, network->t_weights, network->total_weights * sizeof(double));
  memcpy(copy->bias, network->bias, network->total_neurons * sizeof(double));
  memcpy(copy->output, network->output, network->total_neurons * sizeof(double));

  return copy;
}

void neural_free(neural_network_t *network) {
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

static double calculate_cost(neural_network_t *network, int m, const double *y, const double *y_hat) {
  if (!network || !y || !y_hat || m == 0) {
    return NAN;
  }

  double sum = 0.0;
  const double m_inv = 1.0 / (double)m;
  const int total = network->neuron_counts[network->num_hidden_layers + 1] * m;

  // Mean Squared Error (MSE) cost function
  // const double mx2_inv = 0.5 * m_inv;
  // for (int i = 0; i < total; ++i) {
  //   const double diff = y[i] - y_hat[i];
  //   sum += diff * diff;
  // }
  // return mx2_inv * sum;

  // Binary Cross-Entropy (BCE) cost function
  for (int i = 0; i < total; ++i) {
    const double y_i = y[i];
    const double y_hat_i_clamped = fmax(fmin(y_hat[i], 1.0 - 1e-12), 1e-12);

    sum += y_i * log(y_hat_i_clamped) + (1 - y_i) * log(1.0 - y_hat_i_clamped);
  }
  return -m_inv * sum;
}

static void forward_propagate_layer(neural_network_t *network, int in_layer, int m, const double *A_in, double *A_out) {
  int in_size = network->neuron_counts[in_layer];
  int out_size = network->neuron_counts[in_layer + 1];
  int out_offset = neural_layer_offset(network, in_layer + 1);

  const double (*W)[in_size] = neural_layer_t_weights(network, in_layer + 1);
  const double *b = network->bias + out_offset;

  // A[L+1] = sigmoid(Z[L+1]) = W[L+1] * A[L] + b[L+1])
  for (int i = 0; i < out_size; ++i) {
    double *A_out_i = A_out + i * m;
    for (int j = 0; j < m; ++j) {
      A_out_i[j] = b[i];
    }

    // Cache optimized matrix multiplication
    for (int k = 0; k < in_size; ++k) {
      const double W_ik = W[i][k];
      const double *A_in_k = A_in + k * m;
      for (int j = 0; j < m; ++j) {
        A_out_i[j] += W_ik * A_in_k[j];
      }
    }

    for (int j = 0; j < m; ++j) {
      A_out_i[j] = neural_sigmoid(A_out_i[j]);
    }
  }
}

static int neural_layer_offset(neural_network_t *network, int layer) {
  int offset = 0;
  for (int i = 0; i < layer; i++) {
    offset += network->neuron_counts[i];
  }
  return offset;
}

static void calculate_output_layer(neural_network_t *network, int output_layer) {
  int in_size = network->neuron_counts[output_layer - 1];
  int out_size = network->neuron_counts[output_layer];
  int in_offset = neural_layer_offset(network, output_layer - 1);
  int out_offset = in_offset + in_size;

  const double *input = network->output + in_offset;
  double *output = network->output + out_offset;
  const double (*t_weights)[in_size] = neural_layer_t_weights(network, output_layer);
  const double *bias = network->bias + out_offset;

  for (int i = 0; i < out_size; i++) {
    double sum = bias[i];
    for (int j = 0; j < in_size; j++) {
      sum += t_weights[i][j] * input[j];
    }
    output[i] = neural_sigmoid(sum);
  }
}

// Allocate memory for the data used in training and inference.
static char *allocate_data(neural_network_t *network, int m) {
  if (!network || m == 0) {
    return NULL;
  }

  const int num_layers = network->num_hidden_layers + 2;
  const int Y_items = network->neuron_counts[num_layers - 1] * m;
  const int A_matrices_items = network->total_neurons * m;
  const int delta_matrices_items = network->total_neurons * m;
  const int dC_dW_items = network->total_weights;
  const size_t size = (Y_items + A_matrices_items + delta_matrices_items + dC_dW_items) * sizeof(double);
  if (size > network->data_size) {
    if (network->data) {
      free(network->data);
    }
    network->data = malloc(size);
    if (!network->data) {
      network->data_size = 0;
      return NULL;
    }

    network->data_size = size;
  }

  return network->data;
}