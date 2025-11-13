// **********************************************************************
// core/seqgen_utils.h - Shared Signal Generation Utilities
// **********************************************************************
// Sebastian Claudiusz Magierowski Oct 24 2025
//
// Shared utilities for signal generation from DNA sequences, used by both
// ciren_seqgen and sequelizer_seqgen. Provides kseq helpers, sequence
// counting, and signal conversion functions (squiggle -> raw/event).

#ifndef SEQUELIZER_SEQGEN_UTILS_H
#define SEQUELIZER_SEQGEN_UTILS_H

#include "seq_tensor.h"
#include <stdbool.h>

// Pore model parameters
typedef struct {
  const char *model_name;    // e.g., "dna_r10.4.1_e8.2_260bps" or "legacy/legacy_r9.4_180mv_450bps_6mer"
  const char *models_dir;    // Base directory (default: "kmer_models")
  float sample_rate_khz;     // Default: 4.0
} pore_model_params;

// Default parameters constructor
static inline pore_model_params default_pore_model_params(void) {
  pore_model_params params = {
    .model_name = "rna_r9.4_180mv_70bps",  // Small 5-mer default
    .models_dir = "kmer_models",
    .sample_rate_khz = 4.0
  };
  return params;
}

// Generate squiggle from DNA/RNA sequence
// Returns seq_tensor: [n_bases × 3] with columns [current, stddev, dwell]
seq_tensor* sequence_to_squiggle(
  const char *sequence,
  size_t length,
  bool rescale,
  const pore_model_params *params
);

// Convert squiggle to raw signal with Gaussian noise
seq_tensor* squiggle_to_raw(const seq_tensor *squiggle, float sample_rate_khz);

// Convert squiggle to event signal (piecewise constant, no noise)
seq_tensor* squiggle_to_event(const seq_tensor *squiggle, float sample_rate_khz);

// Gaussian random number generator
double gaussian_random(void);

#endif // SEQUELIZER_SEQGEN_UTILS_H
