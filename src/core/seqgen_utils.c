// **********************************************************************
// core/seqgen_utils.c - Shared Signal Generation Utilities
// **********************************************************************
// Sebastian Claudiusz Magierowski Nov 13 2025

#include "seqgen_utils.h"
#include "kmer_model_loader.h"
#include "seq_tensor.h"
#include "seq_utils.h"
#include "../../include/sequelizer.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// **********************************************************************
// Gaussian Random Number Generator
// **********************************************************************

double gaussian_random(void) {
  static int has_spare = 0;
  static double spare;

  if (has_spare) {
      has_spare = 0;
      return spare;
  }

  has_spare = 1;
  double u, v, s;
  do {
      u = (rand() / ((double)RAND_MAX)) * 2.0 - 1.0;
      v = (rand() / ((double)RAND_MAX)) * 2.0 - 1.0;
      s = u * u + v * v;
  } while (s >= 1.0 || s == 0.0);

  s = sqrt(-2.0 * log(s) / s);
  spare = v * s;
  return u * s;
}

// **********************************************************************
// Sequence to Squiggle Conversion
// **********************************************************************

seq_tensor* sequence_to_squiggle(const char *sequence, size_t length, bool rescale, const pore_model_params *params) {
  // Model caching to avoid reloading
  static kmer_model_t *cached_model = NULL;
  static char cached_model_name[256] = {0};

  // Construct full model identifier (name, dir, sample rate)
  char full_model_id[512];
  snprintf(full_model_id, sizeof(full_model_id), "%s/%s", params->models_dir, params->model_name);

  // Check if we need to load a new model 
  if (cached_model == NULL || strcmp(cached_model_name, full_model_id) != 0) {
    if (cached_model) {
      free_kmer_model(cached_model);
    }
    cached_model = load_kmer_model(params->models_dir, params->model_name);
    if (!cached_model) {
      warnx("Failed to load k-mer model: %s", full_model_id);
      return NULL;
    }
    strncpy(cached_model_name, full_model_id, sizeof(cached_model_name) - 1);
    cached_model_name[sizeof(cached_model_name) - 1] = '\0';
  }

  // Encode sequence to k-mer indices
  int kmer_size = cached_model->kmer_size;
  int *kmer_indices = encode_bases_to_integers(sequence, length, kmer_size);
  if (!kmer_indices) {
    warnx("Failed to encode sequence to k-mer indices");
    return NULL;
  }

  size_t num_kmers = length - kmer_size + 1;

  // Create output tensor [num_kmers × 3]
  size_t shape[2] = {num_kmers, 3};
  seq_tensor *squiggle = seq_tensor_create_float(2, shape);
  if (!squiggle) {
    free(kmer_indices);
    warnx("Failed to allocate squiggle tensor");
    return NULL;
  }

  float *data = seq_tensor_data_float(squiggle);

  // Fill squiggle tensor [current, stddev, dwell]
  for (size_t i = 0; i < num_kmers; i++) {
    int kmer_idx = kmer_indices[i];

    // Column 0: current level
    data[i * 3 + 0] = cached_model->level_mean[kmer_idx];

    // Column 1: stddev
    if (cached_model->level_stddev) {
      data[i * 3 + 1] = cached_model->level_stddev[kmer_idx];
    } else {
      data[i * 3 + 1] = cached_model->default_stddev;
    }

    // Column 2: dwell time (samples at reference 4kHz)
    // Simple model: 10 samples per base = 2.5ms at 4kHz
    data[i * 3 + 2] = 10.0f;
  }

  free(kmer_indices);

  // Apply rescaling if requested
  if (rescale) {
    // TODO: Implement median-based normalization of current levels
    // For now, skip rescaling
  }

  return squiggle;
}

// **********************************************************************
// Squiggle to Raw Signal Conversion
// **********************************************************************

seq_tensor* squiggle_to_raw(const seq_tensor *squiggle, float sample_rate_khz) {
  // Validate input
  if (!squiggle || squiggle->ndim != 2 || squiggle->shape[1] != 3) {
    warnx("Invalid squiggle tensor (expected [n × 3])");
    return NULL;
  }

  float *squiggle_data = seq_tensor_data_float((seq_tensor*)squiggle);
  size_t num_events = squiggle->shape[0];

  // Calculate total samples needed
  size_t total_samples = 0;
  for (size_t i = 0; i < num_events; i++) {
    float dwell = squiggle_data[i * 3 + 2];
    // Scale dwell time by sample rate ratio (dwell is at reference 4kHz)
    size_t num_samples = (size_t)ceil(dwell * (sample_rate_khz / 4.0f));
    total_samples += num_samples;
  }

  // Create output tensor [total_samples × 1]
  size_t shape[2] = {total_samples, 1};
  seq_tensor *raw = seq_tensor_create_float(2, shape);
  if (!raw) {
    warnx("Failed to allocate raw signal tensor");
    return NULL;
  }

  float *raw_data = seq_tensor_data_float(raw);

  // Generate raw signal with Gaussian noise
  size_t sample_idx = 0;
  for (size_t i = 0; i < num_events; i++) {
    float current = squiggle_data[i * 3 + 0];
    float stddev = squiggle_data[i * 3 + 1];
    float dwell = squiggle_data[i * 3 + 2];

    size_t num_samples = (size_t)ceil(dwell * (sample_rate_khz / 4.0f));

    // Generate noisy samples for this event
    for (size_t j = 0; j < num_samples && sample_idx < total_samples; j++) {
      raw_data[sample_idx] = current + stddev * gaussian_random();
      sample_idx++;
    }
  }

  return raw;
}

// **********************************************************************
// Squiggle to Event Signal Conversion
// **********************************************************************

seq_tensor* squiggle_to_event(const seq_tensor *squiggle, float sample_rate_khz) {
  // Validate input
  if (!squiggle || squiggle->ndim != 2 || squiggle->shape[1] != 3) {
    warnx("Invalid squiggle tensor (expected [n × 3])");
    return NULL;
  }

  float *squiggle_data = seq_tensor_data_float((seq_tensor*)squiggle);
  size_t num_events = squiggle->shape[0];

  // Calculate total samples needed
  size_t total_samples = 0;
  for (size_t i = 0; i < num_events; i++) {
    float dwell = squiggle_data[i * 3 + 2];
    size_t num_samples = (size_t)ceil(dwell * (sample_rate_khz / 4.0f));
    total_samples += num_samples;
  }

  // Create output tensor [total_samples × 1]
  size_t shape[2] = {total_samples, 1};
  seq_tensor *event = seq_tensor_create_float(2, shape);
  if (!event) {
    warnx("Failed to allocate event signal tensor");
    return NULL;
  }

  float *event_data = seq_tensor_data_float(event);

  // Generate piecewise-constant signal (no noise)
  size_t sample_idx = 0;
  for (size_t i = 0; i < num_events; i++) {
    float current = squiggle_data[i * 3 + 0];
    float dwell = squiggle_data[i * 3 + 2];

    size_t num_samples = (size_t)ceil(dwell * (sample_rate_khz / 4.0f));

    // Generate constant samples for this event (no noise)
    for (size_t j = 0; j < num_samples && sample_idx < total_samples; j++) {
      event_data[sample_idx] = current;
      sample_idx++;
    }
  }

  return event;
}
