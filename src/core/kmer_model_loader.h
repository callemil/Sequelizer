// **********************************************************************
// core/kmer_model_loader.h
// **********************************************************************
// Sebastian Claudiusz Magierowski Nov 12 2025

#ifndef KMER_MODEL_LOADER_H
#define KMER_MODEL_LOADER_H

#include <stddef.h>
#include <stdbool.h>

// K-mer model data structure
typedef struct {
  char *name;                // Model identifier
  int kmer_size;             // 5, 6, or 9
    
  // Required fields
  float *level_mean;         // Array[num_kmers] of mean current levels
    
  // Optional fields
  float *level_stddev;       // Array[num_kmers] or NULL if not in file
  float default_stddev;      // Used when level_stddev == NULL (1.5)
    
  // Legacy extras (parsed but unused initially)
  float *sd_mean;            // NULL if not present
  float *sd_stdv;
  float *ig_lambda;
  float *weight;
    
  size_t num_kmers;          // 4^kmer_size
} kmer_model_t;

// Load model given base directory and model name
// Constructs full path and auto-detects file format
kmer_model_t* load_kmer_model(const char *models_dir, const char *model_name);

// Free a loaded model
void free_kmer_model(kmer_model_t *model);

#endif // KMER_MODEL_LOADER_H