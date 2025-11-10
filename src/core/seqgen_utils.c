// **********************************************************************
// core/seqgen_utils.c - Shared Signal Generation Utilities
// **********************************************************************
// Sebastian Claudiusz Magierowski Oct 24 2025

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <err.h>
#include <stdio.h>
#include <unistd.h>

// Include ciren headers for implementation details
#include "../../../src/ciren_matrix.h"
#include "../../../src/netsiggen.h"
#include "../../../src/ciren_seq_helpers.h"
#include "kseq.h"

// Must declare KSEQ_INIT before including seqgen_utils.h (which uses kseq_t type)
KSEQ_INIT(int, read)

#include "seqgen_utils.h"

// **********************************************************************
// kseq Synthetic Sequence Helpers
// **********************************************************************

// Initialize a kseq_t structure for manual population (without file I/O)
kseq_t* kseq_init_synthetic(void) {
  kseq_t *seq = (kseq_t*)calloc(1, sizeof(kseq_t));
  if (seq == NULL) return NULL;

  // Initialize kstring_t fields to empty state
  seq->name.l = seq->name.m = 0;
  seq->name.s = NULL;
  seq->comment.l = seq->comment.m = 0;
  seq->comment.s = NULL;
  seq->seq.l = seq->seq.m = 0;
  seq->seq.s = NULL;
  seq->qual.l = seq->qual.m = 0;
  seq->qual.s = NULL;
  seq->last_char = 0;
  seq->f = NULL;  // No file stream for synthetic sequences

  return seq;
}

// Set the sequence name in a kseq_t structure
void kseq_set_name(kseq_t *seq, const char *name) {
  size_t len = strlen(name);
  if (seq->name.m <= len) {
    seq->name.m = len + 1;
    seq->name.s = (char*)realloc(seq->name.s, seq->name.m);
  }
  strcpy(seq->name.s, name);
  seq->name.l = len;
}

// Set the sequence data in a kseq_t structure (takes ownership of sequence string)
void kseq_set_seq(kseq_t *seq, char *sequence, size_t length) {
  // Free existing sequence if any
  if (seq->seq.s != NULL) {
    free(seq->seq.s);
  }
  // Take ownership of the provided sequence string
  seq->seq.s = sequence;
  seq->seq.l = length;
  seq->seq.m = length + 1;
}

// Clean up a synthetic kseq_t (simpler than full kseq_destroy since no file stream)
void kseq_destroy_synthetic(kseq_t *seq) {
  if (seq == NULL) return;
  free(seq->name.s);
  free(seq->comment.s);
  free(seq->seq.s);
  free(seq->qual.s);
  free(seq);
}

// **********************************************************************
// File and Sequence Counting
// **********************************************************************

// Count total number of sequences across all input files (FASTA/Q typically hold >1 sequence)
int count_total_sequences(char **files, int nfile) {
  int total = 0;
  for (int i = 0; i < nfile; i++) {
    FILE *fh = fopen(files[i], "r");
    if (NULL == fh) {
      warnx("Failed to open \"%s\" for counting sequences", files[i]);
      continue;
    }
    kseq_t *seq = kseq_init(fileno(fh));
    while (kseq_read(seq) >= 0) {
      total++;
    }
    kseq_destroy(seq);
    fclose(fh);
  }
  return total;
}

// **********************************************************************
// Signal Conversion Utilities
// **********************************************************************

// Convert DNA sequence to squiggle events (current/std/dwell per base)
ciren_matrix sequence_to_squiggle(const char *base_seq, size_t n, bool rescale, const struct squiggle_gen_model_params *params) {
  RETURN_NULL_IF(NULL == base_seq, NULL);
  RETURN_NULL_IF(NULL == params, NULL);

  int *sequence = encode_bases_to_integers(base_seq, n, 1);
  RETURN_NULL_IF(NULL == sequence, NULL);

  squiggle_gen_func_ptr squiggle_function = get_squiggle_gen_func(params->model_type);
  RETURN_NULL_IF(NULL == squiggle_function, NULL);

  ciren_matrix squiggle = squiggle_function(sequence, n, rescale, params);
  free(sequence);

  return squiggle;
}

// Implement Box-Muller transform for Gaussian random numbers
double gaussian_random(void) {
  static int has_spare = 0;
  static double spare;

  if (has_spare) {
    has_spare = 0;
    return spare;
  }

  has_spare = 1;
  static double u, v, s;
  do {
    u = (rand() / ((double) RAND_MAX)) * 2.0 - 1.0;
    v = (rand() / ((double) RAND_MAX)) * 2.0 - 1.0;
    s = u * u + v * v;
  } while (s >= 1.0 || s == 0.0);

  s = sqrt(-2.0 * log(s) / s);
  spare = v * s;
  return u * s;
}

// Convert squiggle events (current/std/dwell per base) to raw signal time-series with Gaussian noise
ciren_matrix squiggle_to_raw(const_ciren_matrix squiggle, float sample_rate_khz) {
  RETURN_NULL_IF(NULL == squiggle, NULL);

  // STEP 1: Calculate total number of samples needed across all squiggle events
  size_t total_samples = 0;
  for (size_t i = 0; i < squiggle->nc; i++) {  // nc = number of squiggle events (bases)
    const size_t offset = i * squiggle->stride;
    const float dwell = squiggle->data.f[offset + 2];  // dwell time for this base
    const size_t num_samples = (size_t)ceil(dwell * (sample_rate_khz / 4.0));  // time -> samples
    total_samples += num_samples;
  }

  // STEP 2: Allocate output matrix for raw signal time-series
  ciren_matrix raw_signal = make_ciren_matrix(total_samples, 1);
  RETURN_NULL_IF(NULL == raw_signal, NULL);

  // STEP 3: Generate raw signal by expanding each squiggle event into multiple samples
  size_t sample_idx = 0;
  for (size_t i = 0; i < squiggle->nc; i++) {  // For each base position
    const size_t offset = i * squiggle->stride;
    const float current = squiggle->data.f[offset + 0];  // Base current level
    const float sd = squiggle->data.f[offset + 1];       // Standard deviation (noise)
    const float dwell = squiggle->data.f[offset + 2];    // Dwell time (residence time)

    const size_t num_samples = (size_t)ceil(dwell * (sample_rate_khz / 4.0));  // Convert time to samples

    // Generate multiple noisy samples for this base's dwell period
    for (size_t j = 0; j < num_samples && sample_idx < total_samples; j++) {
      raw_signal->data.f[sample_idx] = current + sd * gaussian_random();  // Add Gaussian noise
      sample_idx++;
    }
  }

  return raw_signal;
}

// Convert squiggle events to event signal (piecewise-constant, no noise)
ciren_matrix squiggle_to_event(const_ciren_matrix squiggle, float sample_rate_khz) {
  RETURN_NULL_IF(NULL == squiggle, NULL);

  // STEP 1: Calculate total number of samples needed
  size_t total_samples = 0;
  for (size_t i = 0; i < squiggle->nc; i++) {
    const size_t offset = i * squiggle->stride;
    const float dwell = squiggle->data.f[offset + 2];
    const size_t num_samples = (size_t)ceil(dwell * (sample_rate_khz / 4.0));
    total_samples += num_samples;
  }

  // STEP 2: Allocate output matrix
  ciren_matrix event_signal = make_ciren_matrix(total_samples, 1);
  RETURN_NULL_IF(NULL == event_signal, NULL);

  // STEP 3: Generate piecewise-constant signal (no noise)
  size_t sample_idx = 0;
  for (size_t i = 0; i < squiggle->nc; i++) {
    const size_t offset = i * squiggle->stride;
    const float current = squiggle->data.f[offset + 0];
    const float dwell = squiggle->data.f[offset + 2];

    const size_t num_samples = (size_t)ceil(dwell * (sample_rate_khz / 4.0));

    for (size_t j = 0; j < num_samples && sample_idx < total_samples; j++) {
      event_signal->data.f[sample_idx] = current;
      sample_idx++;
    }
  }

  return event_signal;
}
