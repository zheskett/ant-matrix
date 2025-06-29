#include "neural/nn.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "util/util.h"

static double calculate_cost(neural_network_t *network, matrix_t y, matrix_t y_hat);
static void forward_propagate_layer(neural_network_t *network, int layer, const matrix_t A_in, matrix_t A_out);
static void calculate_output_layer(neural_network_t *network, int output_layer);
static void *allocate_data(neural_network_t *network, int m);
// static void *allocate_data_Q(neural_network_t *network, int m);

static inline double neural_sigmoid(double x) { return (x > 45 ? 1.0 : (x < -45 ? -1.0 : 1.0 / (1.0 + exp(-x)))); }
double enc(double x) { return 0.5 * (x + 1.0); }
double dec(double x) { return 2.0 * x - 1.0; }

neural_network_t *neural_create(int num_hidden_layers, const int neuron_counts_array[]) {
  num_hidden_layers = MAX(0, num_hidden_layers);
  neural_network_t *network = malloc(sizeof(neural_network_t));
  if (!network || !neuron_counts_array || num_hidden_layers < 0 || num_hidden_layers > 100) {
    return NULL;
  }

  network->num_hidden_layers = num_hidden_layers;
  network->num_layers = num_hidden_layers + 2;
  network->total_neurons = 0;
  network->total_weights = 0;
  network->data_size = 0;
  network->data = NULL;
  network->neuron_counts = NULL;
  network->output = NULL;
  network->bias = NULL;
  network->weightsT = NULL;

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

  network->neuron_counts = calloc(network->num_layers, sizeof(*network->neuron_counts));
  if (!network->neuron_counts) {
    neural_free(network);
    return NULL;
  }
  memcpy(network->neuron_counts, neuron_counts_array, network->num_layers * sizeof(*network->neuron_counts));

  // Allocate memory for output, weights, and bias
  size_t required_matrices_data_size = (network->total_weights + network->total_neurons * 2) * sizeof(double);
  required_matrices_data_size +=
      (network->num_layers - 1) * sizeof(matrix_t) + network->num_layers * sizeof(vector_t) * 2;

  char *data_ptr = (char *)malloc(required_matrices_data_size);
  if (!data_ptr) {
    neural_free(network);
    return NULL;
  }

  network->output = (vector_t *)data_ptr;
  data_ptr += network->num_layers * sizeof(vector_t);
  network->weightsT = (matrix_t *)data_ptr;
  data_ptr += (network->num_layers - 1) * sizeof(matrix_t);
  network->bias = (vector_t *)data_ptr;
  data_ptr += network->num_layers * sizeof(vector_t);

  // Initialize output, weights, and bias matrices
  for (int i = 0; i < network->num_layers - 1; i++) {
    network->weightsT[i].data = (double *)data_ptr;
    network->weightsT[i].rows = network->neuron_counts[i + 1];
    network->weightsT[i].cols = network->neuron_counts[i];
    data_ptr += network->neuron_counts[i + 1] * network->neuron_counts[i] * sizeof(double);
  }
  for (int i = 0; i < network->num_layers; i++) {
    network->bias[i].data = (double *)data_ptr;
    network->bias[i].rows = network->neuron_counts[i];
    data_ptr += network->neuron_counts[i] * sizeof(double);
  }
  for (int i = 0; i < network->num_layers; i++) {
    network->output[i].data = (double *)data_ptr;
    network->output[i].rows = network->neuron_counts[i];
    data_ptr += network->neuron_counts[i] * sizeof(double);
  }

  // Calculate the size of the data used for training and inference (8 is arbitrary)
  allocate_data(network, INITIAL_DATA_SIZE);
  if (!network->data) {
    neural_free(network);
    return NULL;
  }

  return network;
}

const vector_t *neural_run(neural_network_t *network, const vector_t *input) {
  memcpy(network->output[0].data, input->data, network->neuron_counts[0] * sizeof(double));
  for (int i = 1; i < network->num_layers; i++) {
    calculate_output_layer(network, i);
  }

  return &network->output[network->num_layers - 1];
}

double neural_train(neural_network_t *network, const matrix_t *inputs, const matrix_t *desired_outputs, double lr) {
  if (!network || !inputs || !desired_outputs || inputs->cols != network->neuron_counts[0] ||
      desired_outputs->cols != network->neuron_counts[network->num_layers - 1] ||
      inputs->rows != desired_outputs->rows) {
    return NAN;
  }

  const int m = inputs->rows;
  const double m_inv = 1.0 / (double)m;
  const int num_layers = network->num_layers;
  // Calculate required memory
  const int Y_items = m * network->neuron_counts[num_layers - 1];
  const int L = num_layers - 1;

  double cost = 0.0;
  double *data_ptr = (double *)allocate_data(network, m);
  matrix_t Y = {data_ptr, network->neuron_counts[num_layers - 1], m};
  matrix_t A[num_layers];
  matrix_t delta[num_layers];
  matrix_t dC_dW[num_layers - 1];

  if (!data_ptr) {
    return NAN;
  }

  // Allocate memory for A, delta, dC_db, and dC_dW
  data_ptr += Y_items;
  for (int i = 0; i < num_layers; i++) {
    A[i].data = data_ptr;
    A[i].rows = network->neuron_counts[i];
    A[i].cols = m;
    data_ptr += network->neuron_counts[i] * m;
  }
  for (int i = 0; i < num_layers; i++) {
    delta[i].data = data_ptr;
    delta[i].rows = network->neuron_counts[i];
    delta[i].cols = m;
    data_ptr += network->neuron_counts[i] * m;
  }
  for (int i = 0; i < num_layers - 1; i++) {
    dC_dW[i].data = data_ptr;
    dC_dW[i].rows = network->neuron_counts[i + 1];
    dC_dW[i].cols = network->neuron_counts[i];
    data_ptr += network->neuron_counts[i + 1] * network->neuron_counts[i];
  }

  // Transpose inputs, desired_outputs for Y and A[0]
  // #pragma omp parallel for
  for (int j = 0; j < m; j++) {
    const double *do_j = desired_outputs->data + j * network->neuron_counts[L];
    const double *in_j = inputs->data + j * network->neuron_counts[0];
    for (int i = 0; i < network->neuron_counts[L]; i++) {
      Y.data[i * m + j] = do_j[i];
    }

    for (int i = 0; i < network->neuron_counts[0]; i++) {
      A[0].data[i * m + j] = in_j[i];
    }
  }

  // Initialize all of delta to zero
  memset(delta[0].data, 0, network->total_neurons * m * sizeof(double));

  // Feed forward through the network
  for (int i = 0; i < L; i++) {
    forward_propagate_layer(network, i, A[i], A[i + 1]);
  }

  cost = calculate_cost(network, Y, A[L]);

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
  double *bias_L = network->bias[L].data;
  for (int i = 0; i < network->neuron_counts[L]; i++) {
    double sum = 0.0;
    double *delta_Li = delta[L].data + i * m;
    const double *A_Li = A[L].data + i * m;
    const double *Y_i = Y.data + i * m;
    for (int k = 0; k < m; k++) {
      const double value = m_inv * (A_Li[k] - Y_i[k]);
      delta_Li[k] = value;
      sum += value;
    }
    // Gradient descent bias
    bias_L[i] -= lr * sum;
  }

  // dC/dW^L = (delta^L)((A^{L-1})^T)
  matrix_multiply_transformB(&delta[L], &A[L - 1], &dC_dW[L - 1], true);

  // Hidden layers (1 - L-1)
  for (int l = num_layers - 2; l >= 1; l--) {
    // dC/dA^[l] = (W^[l+1]^T)(delta^[l+1])
    const matrix_t W_l1_mtr = neural_layer_weightsT(network, l + 1);
    const matrix_t delta_l1 = delta[l + 1];
    matrix_t delta_l = delta[l];
    const vector_t bias_l = network->bias[l];
    const matrix_t A_l = A[l];

    matrix_multiply_transformA(&W_l1_mtr, &delta_l1, &delta_l, false);

    // Cache optimized order
    // #pragma omp parallel for
    for (int i = 0; i < A_l.rows; i++) {
      double *delta_li = delta_l.data + i * m;
      const double *A_li = A_l.data + i * m;
      double sum = 0.0;

      // delta^l = dC/dA^[l] * dA^[l]/dZ^[l]
      for (int j = 0; j < m; j++) {
        delta_li[j] *= (A_li[j] * (1 - A_li[j]));
        sum += delta_li[j];
      }

      // Gradient descent bias
      bias_l.data[i] -= lr * sum;
    }

    // dC/dW^l = (delta^l)((A^{l-1})^T)
    matrix_multiply_transformB(&delta_l, &A[l - 1], &dC_dW[l - 1], true);
  }

  // Apply gradient descent on weights
  // #pragma omp parallel for
  for (int i = 0; i < network->total_weights; i++) {
    network->weightsT[0].data[i] -= lr * dC_dW[0].data[i];
  }

  return cost;
}

void neural_randomize_weights(neural_network_t *network, double min_weight, double max_weight) {
  if (!network || !network->weightsT) {
    return;
  }

  for (int i = 0; i < network->total_weights; i++) {
    network->weightsT[0].data[i] = min_weight + (max_weight - min_weight) * ((double)rand() / RAND_MAX);
  }
}

void neural_randomize_bias(neural_network_t *network, double min_bias, double max_bias) {
  if (!network || !network->bias) {
    return;
  }

  for (int i = 0; i < network->total_neurons; i++) {
    network->bias[0].data[i] = min_bias + (max_bias - min_bias) * ((double)rand() / RAND_MAX);
  }
}

matrix_t neural_layer_weightsT(neural_network_t *network, int out_layer) { return network->weightsT[out_layer - 1]; }

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
    matrix_t layer_weights = neural_layer_weightsT(network, i + 1);
    for (int j = 0; j < network->neuron_counts[i]; j++) {
      fprintf(fp, "[");
      for (int k = 0; k < network->neuron_counts[i + 1]; k++) {
        if (k > 0) {
          fprintf(fp, ", ");
        }
        fprintf(fp, "% .2f", matrix_get(&layer_weights, k, j));
      }
      fprintf(fp, "]\n");
    }
  }

  fprintf(fp, "\nBiases:\n");
  for (int i = 1; i < network->num_hidden_layers + 2; i++) {
    fprintf(fp, "Layer %d: ", i);
    double *bias = network->bias[i].data;
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
    double *output = network->output[i].data;
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

/* bool neural_write(neural_network_t *network, const char *filename) {
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

  // Write neuron counts, weightsT, bias. Ignore output/data
  fwrite(network->neuron_counts, sizeof(int), network->num_hidden_layers + 2, fp);
  fwrite(network->weightsT, sizeof(double), network->total_weights, fp);
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
  if (ferror(fp) || feof(fp)) {
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
  if (ferror(fp) || feof(fp)) {
    free(neuron_counts);
    fclose(fp);
    fprintf(stderr, "Could not read file %s", filename);
    return NULL;
  }

  double *weightsT = malloc(total_weights * sizeof(double));
  if (!weightsT) {
    free(neuron_counts);
    fclose(fp);
    return NULL;
  }
  fread(weightsT, sizeof(double), total_weights, fp);
  if (ferror(fp) || feof(fp)) {
    free(neuron_counts);
    free(weightsT);
    fclose(fp);
    fprintf(stderr, "Could not read file %s", filename);
    return NULL;
  }

  double *bias = malloc(total_neurons * sizeof(double));
  if (!bias) {
    free(neuron_counts);
    free(weightsT);
    fclose(fp);
    return NULL;
  }
  fread(bias, sizeof(double), total_neurons, fp);
  if (ferror(fp)) {
    free(neuron_counts);
    free(weightsT);
    free(bias);
    fclose(fp);
    fprintf(stderr, "Could not read file %s", filename);
    return NULL;
  }

  fclose(fp);

  neural_network_t *network = malloc(sizeof(neural_network_t));
  if (!network) {
    free(neuron_counts);
    free(weightsT);
    free(bias);
    return NULL;
  }

  network->num_hidden_layers = num_hidden_layers;
  network->total_neurons = total_neurons;
  network->total_weights = total_weights;
  network->neuron_counts = neuron_counts;
  network->output = calloc(total_neurons, sizeof(double));
  network->weightsT = weightsT;
  network->bias = bias;

  network->data = NULL;
  network->data_size = 0;
  allocate_data(network, INITIAL_DATA_SIZE);
  if (!network->data) {
    neural_free(network);
    return NULL;
  }

  return network;
} */

void neural_free(neural_network_t *network) {
  if (!network) {
    return;
  }

  if (network->neuron_counts) {
    free(network->neuron_counts);
    network->neuron_counts = NULL;
  }
  if (network->output) {
    free(network->output);
    network->output = NULL;
  }
  if (network->data) {
    free(network->data);
    network->data = NULL;
  }
  network->weightsT = NULL;
  network->bias = NULL;
  network->num_hidden_layers = 0;
  network->total_neurons = 0;
  network->total_weights = 0;
  network->num_layers = 0;
  network->data_size = 0;
  free(network);
}

static double calculate_cost(neural_network_t *network, const matrix_t y, const matrix_t y_hat) {
  if (!network || !y.data || !y_hat.data || y.rows != y_hat.rows || y.cols != y_hat.cols) {
    return NAN;
  }

  int m = y.cols;
  double sum = 0.0;
  const double m_inv = 1.0 / (double)m;
  const int total = y.rows * m;

  // Mean Squared Error (MSE) cost function
  // const double mx2_inv = 0.5 * m_inv;
  // for (int i = 0; i < total; ++i) {
  //   const double diff = y[i] - y_hat[i];
  //   sum += diff * diff;
  // }
  // return mx2_inv * sum;

  // Binary Cross-Entropy (BCE) cost function
  for (int i = 0; i < total; ++i) {
    const double y_i = y.data[i];
    const double y_hat_i_clamped = fmax(fmin(y_hat.data[i], 1.0 - 1e-12), 1e-12);

    sum += y_i * log(y_hat_i_clamped) + (1 - y_i) * log(1.0 - y_hat_i_clamped);
  }
  return -m_inv * sum;
}

static void forward_propagate_layer(neural_network_t *network, int in_layer, const matrix_t A_in, matrix_t A_out) {
  const int m = A_in.cols;
  const matrix_t W = neural_layer_weightsT(network, in_layer + 1);
  const vector_t b = network->bias[in_layer + 1];

  // A[L+1] = sigmoid(Z[L+1]) = W[L+1] * A[L] + b[L+1])
  for (int i = 0; i < A_out.rows; i++) {
    double *A_out_i = A_out.data + i * m;
    for (int j = 0; j < m; ++j) {
      A_out_i[j] = b.data[i];
    }
  }

  matrix_multiply_append(&W, &A_in, &A_out);
  for (int i = 0; i < A_out.rows; i++) {
    double *A_out_i = A_out.data + i * m;
    for (int j = 0; j < m; ++j) {
      A_out_i[j] = neural_sigmoid(A_out_i[j]);
    }
  }
}

static void calculate_output_layer(neural_network_t *network, int output_layer) {
  int in_size = network->neuron_counts[output_layer - 1];
  int out_size = network->neuron_counts[output_layer];

  const double *input = network->output[output_layer - 1].data;
  double *output = network->output[output_layer].data;
  const matrix_t weightsT = neural_layer_weightsT(network, output_layer);
  const double *bias = network->bias[output_layer].data;

  for (int i = 0; i < out_size; i++) {
    double sum = bias[i];
    for (int j = 0; j < in_size; j++) {
      sum += weightsT.data[i * weightsT.cols + j] * input[j];
    }
    output[i] = neural_sigmoid(sum);
  }
}

// Allocate memory for the data used in training and inference.
static void *allocate_data(neural_network_t *network, int m) {
  if (!network || m == 0) {
    return NULL;
  }

  const int num_layers = network->num_layers;
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

// static void *allocate_data_Q(neural_network_t *network, int m) {
//   return NULL:
// }