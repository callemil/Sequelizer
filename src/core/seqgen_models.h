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
  SEQGEN_MODEL_R9_4,
  SEQGEN_MODEL_R9_4_RNA,
  SEQGEN_MODEL_R10,
  SEQGEN_MODEL_KMER,
  SEQGEN_MODEL_INVALID
} seqgen_model_type;

// **********************************************************************
// Model Parameters
// **********************************************************************

// USE k-mer model (may differ from LOAD k-mer model in kmer_model_loader.h by chosen k-mer size)
struct kmer_gen_model_params {
  const char *model_name;     // e.g., "dna_r10.4.1_e8.2_260bps"
  const char *models_dir;     // base directory (default: "kmer_models")
  int kmer_size;              // k-mer size: CAN CHOOSE if not > kmer_size of model you LOAD (5, 6, or 9)
  float sample_rate_khz;      // Default: 4.0
};

struct neural_gen_model_params {
  int placeholder;            // for future use
};

struct seqgen_model_params {
  seqgen_model_type model_type;
  union {
    struct neural_gen_model_params neural;
    struct kmer_gen_model_params kmer;
  } params;
};

// **********************************************************************
// Function Pointer Interface
// **********************************************************************

// Squiggle generation function pointer type
// Takes: encoded sequence (int array), length, transform_units flag, model params
// Returns: seq_tensor [n_kmers × 3] with columns [current, stddev, dwell]
typedef seq_tensor* (*seqgen_func_ptr)(
  int const * sequence,
  size_t n,
  bool transform_units,
  const struct seqgen_model_params * params
);

// **********************************************************************
// Dispatcher
// **********************************************************************

// Convert model name string to enum value
seqgen_model_type get_seqgen_model(const char *model_str);

// Get the appropriate squiggle generation function for a model type
// Returns NULL if model_type is invalid
seqgen_func_ptr get_seqgen_func(const seqgen_model_type model_type);

// **********************************************************************
// Model-Specific Functions
// **********************************************************************

// Neural network implementations (future)
seq_tensor* squiggle_r94(int const * sequence, size_t n, bool transform_units, const struct seqgen_model_params * params);
seq_tensor* squiggle_r94_rna(int const * sequence, size_t n, bool transform_units, const struct seqgen_model_params * params);
seq_tensor* squiggle_r10(int const * sequence, size_t n, bool transform_units, const struct seqgen_model_params * params);

// K-mer lookup implementation
seq_tensor* squiggle_kmer(int const * sequence, size_t n, bool transform_units, const struct seqgen_model_params * params);

#endif // SEQGEN_MODELS_H
