// **********************************************************************
// core/seqgen_models.c - Signal Generation Model Implementation
// **********************************************************************
// Sebastian Claudiusz Magierowski Nov 15 2025

#include "seqgen_models.h"
#include "kmer_model_loader.h"
#include "seq_utils.h"
#include "../../include/sequelizer.h"
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <math.h>

// **********************************************************************
// String to Model Type Conversion
// **********************************************************************

seqgen_model_type get_seqgen_model(const char *model_str) {
  if (0 == strcmp(model_str, "squiggle_r94")) {
    return SEQGEN_MODEL_R9_4;
  }
  if (0 == strcmp(model_str, "squiggle_r94_rna")) {
    return SEQGEN_MODEL_R9_4_RNA;
  }
  if (0 == strcmp(model_str, "squiggle_r10")) {
    return SEQGEN_MODEL_R10;
  }
  if (0 == strcmp(model_str, "squiggle_kmer")) {
    return SEQGEN_MODEL_KMER;
  }
  return SEQGEN_MODEL_INVALID;
}

// **********************************************************************
// Model Dispatcher
// **********************************************************************

seqgen_func_ptr get_seqgen_func(const seqgen_model_type model_type) {
  switch (model_type) {
    case SEQGEN_MODEL_R9_4:
      return squiggle_r94;
    case SEQGEN_MODEL_R9_4_RNA:
      return squiggle_r94_rna;
    case SEQGEN_MODEL_R10:
      return squiggle_r10;
    case SEQGEN_MODEL_KMER:
      return squiggle_kmer;
    case SEQGEN_MODEL_INVALID:
      errx(EXIT_FAILURE, "Invalid sequelizer squiggle model %s:%d", __FILE__, __LINE__);
    default:
      errx(EXIT_FAILURE, "Sequelizer enum failure -- report bug\n");
  }
  return NULL;
}

// **********************************************************************
// K-mer Lookup Model Implementation
// **********************************************************************

seq_tensor* squiggle_kmer(int const * sequence, size_t n, bool transform_units, const struct seqgen_model_params * params) {
  if (!sequence || !params) return NULL;

  // Extract k-mer model parameters
  const char *model_name = params->params.kmer.model_name;
  const char *models_dir = params->params.kmer.models_dir;
  int kmer_size = params->params.kmer.kmer_size;
  float sample_rate_khz = params->params.kmer.sample_rate_khz;

  // Validate k-mer size
  if (kmer_size <= 0 || kmer_size > 9) {
    warnx("K-mer size %d out of range [1-9]", kmer_size);
    return NULL;
  }

  // Ensure sequence is long enough
  if (n < (size_t)kmer_size) {
    warnx("Sequence length %zu shorter than k-mer size %d", n, kmer_size);
    return NULL;
  }

  // Load model (with static caching to avoid reloading)
  static kmer_model_t *cached_model = NULL;
  static char cached_model_id[512] = {0};

  char model_id[512];
  snprintf(model_id, sizeof(model_id), "%s/%s", models_dir, model_name);

  if (!cached_model || strcmp(cached_model_id, model_id) != 0) {
    if (cached_model) {
      free_kmer_model(cached_model);
    }
    cached_model = load_kmer_model(models_dir, model_name);
    if (!cached_model) {
      warnx("Failed to load k-mer model: %s", model_id);
      return NULL;
    }
    strncpy(cached_model_id, model_id, sizeof(cached_model_id) - 1);
    cached_model_id[sizeof(cached_model_id) - 1] = '\0';
  }

  // Check if loaded model matches requested k-mer size
  int loaded_kmer_size = cached_model->kmer_size;

  // Create lookup table (either use directly or decimate)
  float *lookup_mean = NULL;
  float *lookup_stddev = NULL;
  int num_kmers = 1;
  for (int i = 0; i < kmer_size; i++) num_kmers *= 4;

  if (kmer_size == loaded_kmer_size) {
    // Direct use - no decimation needed
    lookup_mean = cached_model->level_mean;
    lookup_stddev = cached_model->level_stddev;
  } else if (kmer_size < loaded_kmer_size) {
    // Decimate: average multiple k-mers from larger model
    int decimation_factor = 1;
    for (int i = 0; i < (loaded_kmer_size - kmer_size); i++) {
      decimation_factor *= 4;
    }

    lookup_mean = calloc(num_kmers, sizeof(float));
    if (cached_model->level_stddev) {
      lookup_stddev = calloc(num_kmers, sizeof(float));
    }

    if (!lookup_mean || (cached_model->level_stddev && !lookup_stddev)) {
      free(lookup_mean);
      free(lookup_stddev);
      return NULL;
    }

    // Average groups of k-mers
    for (int k_idx = 0; k_idx < num_kmers; k_idx++) {
      float sum_mean = 0.0f;
      float sum_stddev = 0.0f;
      int base_idx = k_idx * decimation_factor;

      for (int i = 0; i < decimation_factor; i++) {
        sum_mean += cached_model->level_mean[base_idx + i];
        if (cached_model->level_stddev) {
          sum_stddev += cached_model->level_stddev[base_idx + i];
        }
      }

      lookup_mean[k_idx] = sum_mean / decimation_factor;
      if (lookup_stddev) {
        lookup_stddev[k_idx] = sum_stddev / decimation_factor;
      }
    }
  } else {
    // kmer_size > loaded_kmer_size - not supported
    warnx("Requested k-mer size %d larger than loaded model size %d", kmer_size, loaded_kmer_size);
    return NULL;
  }

  // Calculate number of k-mers in sequence
  size_t num_sequence_kmers = n - kmer_size + 1;

  // Create output tensor [num_sequence_kmers × 3]
  size_t shape[2] = {num_sequence_kmers, 3};
  seq_tensor *result = seq_tensor_create_float(2, shape);
  if (!result) {
    if (kmer_size != loaded_kmer_size) {
      free(lookup_mean);
      free(lookup_stddev);
    }
    return NULL;
  }

  float *data = seq_tensor_data_float(result);

  // Process each k-mer position
  for (size_t i = 0; i < num_sequence_kmers; i++) {
    // Convert k-mer to index (base-4 encoding)
    int kmer_index = 0;
    for (int j = 0; j < kmer_size; j++) {
      int base = sequence[i + j];
      if (base < 0 || base > 3) {
        seq_tensor_free(result);
        if (kmer_size != loaded_kmer_size) {
          free(lookup_mean);
          free(lookup_stddev);
        }
        warnx("Invalid base %d at position %zu", base, i + j);
        return NULL;
      }
      kmer_index = kmer_index * 4 + base;
    }

    // Populate row [current, stddev, dwell]
    data[i * 3 + 0] = lookup_mean[kmer_index];

    if (lookup_stddev) {
      data[i * 3 + 1] = lookup_stddev[kmer_index];
    } else {
      data[i * 3 + 1] = cached_model->default_stddev;
    }

    data[i * 3 + 2] = 10.0f * (sample_rate_khz / 4.0f);
  }

  // Clean up decimated lookup tables if we created them
  if (kmer_size != loaded_kmer_size) {
    free(lookup_mean);
    free(lookup_stddev);
  }

  return result;
}

// **********************************************************************
// Neural Network Model Stubs (Future Implementation)
// **********************************************************************

seq_tensor* squiggle_r94(int const * sequence, size_t n, bool transform_units, const struct seqgen_model_params * params) {
  warnx("Neural network model R9.4 not yet implemented in sequelizer");
  return NULL;
}

seq_tensor* squiggle_r94_rna(int const * sequence, size_t n, bool transform_units, const struct seqgen_model_params * params) {
  warnx("Neural network model R9.4 RNA not yet implemented in sequelizer");
  return NULL;
}

seq_tensor* squiggle_r10(int const * sequence, size_t n, bool transform_units, const struct seqgen_model_params * params) {
  warnx("Neural network model R10 not yet implemented in sequelizer");
  return NULL;
}
