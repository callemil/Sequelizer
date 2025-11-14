// **********************************************************************
// core/seqgen_models.h - Signal Generation Model Dispatcher
// **********************************************************************
// Sebastian Claudiusz Magierowski Nov 14 2025
//
// Dispatcher pattern for signal generation models. Allows different model
// types (k-mer lookup, neural networks) to be used through a unified
// function pointer interface.

#ifndef SEQGEN_MODELS_H
#define SEQGEN_MODELS_H

#include "seq_tensor.h"
#include <stdbool.h>
#include <stddef.h>

// **********************************************************************
// Model Type Enumeration
// **********************************************************************

// Supported signal generation model types
typedef enum {
  SEQGEN_MODEL_KMER,      // K-mer lookup table models
  SEQGEN_MODEL_NEURAL,    // Neural network models (future)
  SEQGEN_MODEL_INVALID
} seqgen_model_type;

// **********************************************************************
// Model Parameters
// **********************************************************************

// K-mer model parameters
typedef struct {
  const char *model_name;     // e.g., "dna_r10.4.1_e8.2_260bps"
  const char *models_dir;     // Base directory (default: "kmer_models")
  float sample_rate_khz;      // Default: 4.0
} kmer_model_params;

// Neural network model parameters (placeholder for future)
typedef struct {
  const char *model_path;     // Path to neural network weights
  int placeholder;            // To be defined when implementing
} neural_model_params;

// Unified model parameters (tagged union)
typedef struct {
  seqgen_model_type model_type;
  union {
    kmer_model_params kmer;
    neural_model_params neural;
  } params;
} seqgen_model_params;

// **********************************************************************
// Function Pointer Interface
// **********************************************************************

// Squiggle generation function pointer type
// Takes: encoded sequence (int array), length, rescale flag, model params
// Returns: seq_tensor [n_kmers × 3] with columns [current, stddev, dwell]
typedef seq_tensor* (*seqgen_func_ptr)(
  const int *encoded_sequence,
  size_t length,
  bool rescale,
  const void *model_params
);

// **********************************************************************
// Dispatcher
// **********************************************************************

// Get the appropriate squiggle generation function for a model type
// Returns NULL if model_type is invalid
seqgen_func_ptr get_seqgen_func(seqgen_model_type model_type);

// **********************************************************************
// Model-Specific Functions (declared here, implemented elsewhere)
// **********************************************************************

// K-mer lookup implementation (in seqgen_utils.c)
seq_tensor* kmer_lookup_squiggle(
  const int *encoded_sequence,
  size_t length,
  bool rescale,
  const void *params  // Actually kmer_model_params*
);

// Neural network implementation (future - in seqgen_neural.c or similar)
seq_tensor* neural_net_squiggle(
  const int *encoded_sequence,
  size_t length,
  bool rescale,
  const void *params  // Actually neural_model_params*
);

#endif // SEQGEN_MODELS_H
