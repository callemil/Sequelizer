// **********************************************************************
// core/siggen_utils.h - Shared Signal Generation Utilities
// **********************************************************************
// Sebastian Claudiusz Magierowski Nov 12 2025
//
// Shared utilities for signal generation from DNA sequences, used by both
// ciren_seqgen and sequelizer_seqgen. Provides kseq helpers, sequence
// counting, and signal conversion functions (squiggle -> raw/event).

#ifndef SEQUELIZER_SIGGEN_UTILS_H
#define SEQUELIZER_SIGGEN_UTILS_H

#include <stddef.h>
#include <stdbool.h>

// IMPORTANT: Before including this header, you must include:
// - ciren_matrix.h (defines ciren_matrix and const_ciren_matrix)
// - netsiggen.h (defines struct squiggle_gen_model_params, also brings in RETURN_NULL_IF via ciren.h)
// - kseq.h with KSEQ_INIT() called (defines kseq_t type)
//
// NOTE: RETURN_NULL_IF macro comes indirectly via netsiggen.h → ciren.h → sequelizer.h
// This is intentional to avoid direct circular dependencies.

// **********************************************************************
// kseq Synthetic Sequence Helpers
// **********************************************************************
// These functions allow synthetic sequence generation to use the same
// kseq_t structure as file-based sequences, ensuring structural identity
// and compatibility with bioinformatics conventions.

// Initialize a kseq_t structure for manual population (without file I/O)
kseq_t* kseq_init_synthetic(void);

// Set the sequence name in a kseq_t structure
void kseq_set_name(kseq_t *seq, const char *name);

// Set the sequence data in a kseq_t structure (takes ownership of sequence string)
void kseq_set_seq(kseq_t *seq, char *sequence, size_t length);

// Clean up a synthetic kseq_t (simpler than full kseq_destroy since no file stream)
void kseq_destroy_synthetic(kseq_t *seq);

// **********************************************************************
// File and Sequence Counting
// **********************************************************************

// Count total number of sequences across all input files
// Note: FASTA/FASTQ files typically hold multiple sequences
int count_total_sequences(char **files, int nfile);

// **********************************************************************
// Signal Conversion Utilities
// **********************************************************************

// Convert DNA sequence to squiggle events (current/std/dwell per base)
// Uses model-specific squiggle function based on params->model_type
ciren_matrix sequence_to_squiggle(
  const char *base_seq,
  size_t n,
  bool rescale,
  const struct squiggle_gen_model_params *params
);

// Generate Gaussian random value using Box-Muller transform
// Used for adding realistic noise to raw signals
double gaussian_random(void);

// Convert squiggle events to raw signal time-series with Gaussian noise
// sample_rate_khz: sampling rate in kilohertz (e.g., 4.0 for 4 kHz)
ciren_matrix squiggle_to_raw(const_ciren_matrix squiggle, float sample_rate_khz);

// Convert squiggle events to event signal (piecewise-constant, no noise)
// sample_rate_khz: sampling rate in kilohertz (e.g., 4.0 for 4 kHz)
ciren_matrix squiggle_to_event(const_ciren_matrix squiggle, float sample_rate_khz);

#endif // SEQUELIZER_SIGGEN_UTILS_H
