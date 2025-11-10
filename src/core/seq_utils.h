// **********************************************************************
// sequelizer/src/core/seq_utils.h - DNA Sequence Utilities
// **********************************************************************
// Sebastian Claudiusz Magierowski Jun 8 2023
// Moved to sequelizer core: Nov 9 2025

#ifndef SEQUELIZER_SEQ_UTILS_H
#define SEQUELIZER_SEQ_UTILS_H

char* random_str(int len);

char* random_str_seed(int len, unsigned int seed);

char** random_str_batch(int len, int num_examples);

char** random_str_batch_seed(int len, int num_examples, unsigned int seed);

char** seq_kmers(const char* sequence, int k, int* num_kmers);

void free_kmers(char** kmers, int num_kmers);

int kmer_to_int_rev(const char* kmer);

int kmer_to_int(const char* kmer);

void int_to_kmer(int len, int index, char* kmer);

int* seq_kmers_to_ints(const char* sequence, int k, int* num_ints);

#endif // SEQUELIZER_SEQ_UTILS_H
