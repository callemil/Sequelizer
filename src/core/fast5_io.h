// **********************************************************************
// core/fast5_io.h - Fast5 File I/O Operations for Sequelizer
// **********************************************************************
// S Magierowski Aug 3 2025
//
#ifndef SEQUELIZER_FAST5_IO_H
#define SEQUELIZER_FAST5_IO_H

#include <hdf5.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "fast5_utils.h"

// Fast5 file discovery functions
bool is_fast5_file(const char *filename);
char** find_fast5_files_recursive(const char *directory, size_t *count);
char** find_fast5_files(const char *input_path, bool recursive, size_t *count);
void free_file_list(char **files, size_t count);

// Enhanced file validation functions (from ciren)
bool file_is_accessible(const char *filename);
bool is_likely_fast5_file(const char *filename);
bool is_valid_hdf5_file(const char *filename);
bool has_fast5_structure(const char *filename);

// Fast5 file reading functions
fast5_metadata_t* read_fast5_metadata(const char *filename, size_t *metadata_count);
void free_fast5_metadata(fast5_metadata_t *metadata, size_t count);

// Signal extraction functions
float* read_fast5_signal(const char *filename, const char *read_id, size_t *signal_length);
void free_fast5_signal(float *signal);

typedef void (*metadata_enhancer_t)(hid_t file_id, hid_t signal_dataset, fast5_metadata_t *metadata);
fast5_metadata_t* read_fast5_metadata_with_enhancer(const char *filename, size_t *metadata_count, metadata_enhancer_t enhancer);

#endif // SEQUELIZER_FAST5_IO_H