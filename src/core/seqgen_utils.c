// **********************************************************************
// core/seqgen_utils.c - Shared Signal Generation Utilities
// **********************************************************************
// Sebastian Claudiusz Magierowski Nov 13 2025

#include "seqgen_utils.h"
#include "seqgen_models.h"
#include "seq_tensor.h"
#include "seq_utils.h"
#include "../../include/sequelizer.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

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
// Sequence to Squiggle Conversion (High-Level Wrapper)
// **********************************************************************

seq_tensor* sequence_to_squiggle(
  const char *sequence,
  size_t length,
  bool rescale,
  const struct seqgen_model_params *params
) {
  if (!sequence || !params) {
    warnx("Invalid parameters to sequence_to_squiggle");
    return NULL;
  }

  // Encode sequence to integer array of individual bases (A=0, C=1, G=2, T=3)
  // Note: squiggle_kmer() expects individual base values, not k-mer indices
  int *encoded = calloc(length, sizeof(int));
  if (!encoded) {
    warnx("Failed to allocate memory for sequence encoding");
    return NULL;
  }

  for (size_t i = 0; i < length; i++) {
    encoded[i] = base_to_int(sequence[i], true);
    if (encoded[i] < 0) {
      warnx("Invalid base '%c' at position %zu", sequence[i], i);
      free(encoded);
      return NULL;
    }
  }

  // Get appropriate model function via dispatcher
  seqgen_func_ptr func = get_seqgen_func(params->model_type);
  if (!func) {
    warnx("Failed to get model function for type %d", params->model_type);
    free(encoded);
    return NULL;
  }

  // Call model-specific function
  seq_tensor *squiggle = func(encoded, length, rescale, params);

  // Clean up
  free(encoded);

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
