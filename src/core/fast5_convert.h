// **********************************************************************
// core/fast5_convert.h - Fast5 Format Conversion Functions
// **********************************************************************
// Sebastian Claudiusz Magierowski Aug 22 2025
//

#ifndef SEQUELIZER_FAST5_CONVERT_H
#define SEQUELIZER_FAST5_CONVERT_H

#include <stdbool.h>
#include <stddef.h>
#include "fast5_utils.h"

// **********************************************************************
// Signal Extraction Functions
// **********************************************************************

// Write signal data to text file with metadata header (one sample per line)
int write_signal_to_file(const char *filename, float *signal, size_t signal_length, const fast5_metadata_t *metadata);

// Create output directory if it doesn't exist
int create_directory(const char *path);

// Extract raw signals from Fast5 files
int extract_raw_signals(char **files, size_t file_count, const char *output_file, 
                        bool all_reads, bool verbose);

// **********************************************************************
// Metadata Extraction Functions  
// **********************************************************************

// Extract metadata to text format
int extract_metadata(char **files, size_t file_count, const char *output_file, bool verbose);

#endif // SEQUELIZER_FAST5_CONVERT_H