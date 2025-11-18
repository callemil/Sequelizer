// **********************************************************************
// core/seqgen_utils.h - Shared Signal Generation Utilities
// **********************************************************************
// Sebastian Claudiusz Magierowski Oct 24 2025
/*
Shared utilities for signal generation from DNA sequences, used by both
 ciren_seqgen and sequelizer_seqgen. Provides kseq helpers, sequence
 counting, and signal conversion functions (squiggle -> raw/event).

 user -> sequence_to_squiggle()               -> squiggle_kmer() -> seq_tensor [n_kmers x 3]
           encode,                           '-> squiggle_r94() '
           get_seqgen_func() (dispatcher)    
*/
#ifndef SEQUELIZER_SEQGEN_UTILS_H
#define SEQUELIZER_SEQGEN_UTILS_H

#include "seq_tensor.h"
#include "seqgen_models.h"
#include <stdbool.h>

// Generate squiggle from DNA/RNA sequence (high-level wrapper)
// Returns seq_tensor: [n_kmers × 3] with columns [current, stddev, dwell]
seq_tensor* sequence_to_squiggle(
  const char *sequence,
  size_t length,
  bool rescale,
  const struct seqgen_model_params *params
);

// Convert squiggle to raw signal with Gaussian noise
seq_tensor* squiggle_to_raw(const seq_tensor *squiggle, float sample_rate_khz);

// Convert squiggle to event signal (piecewise constant, no noise)
seq_tensor* squiggle_to_event(const seq_tensor *squiggle, float sample_rate_khz);

// Gaussian random number generator
double gaussian_random(void);

#endif // SEQUELIZER_SEQGEN_UTILS_H
