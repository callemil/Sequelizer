// **********************************************************************
// core/fast5_convert.h - Fast5 Format Conversion Functions
// **********************************************************************
// Sebastian Claudiusz Magierowski Aug 22 2025
/*
 Format conversion utilities (Fast5 to other formats).

 Default single-read file o/p name: read_ch<channel num>_rd<read num>.txt
 Default multi-read file o/p name:  <original fast5 files name>_read_ch<channe _num>_rd<read num>.txt

 Get help:            sequelizer convert --help
 Convert to raw:      sequelizer convert data.fast5 --to raw
 Wordy convert:       sequelizer convert data.fast5 --to raw --verbose
 Extract *all* reads: sequelizer convert multi-read.fast4 --to raw --all

 TODO:
 sequelizer convert input.fast5 --to slow5 -o output.slow5                      # to slow5 format
 sequelizer convert input.fast5 --to metadata --format json -o metadata.json    # extract metadata to JSON
 sequelizer convert input.fast5 --to metadata --format csv -o metadata.csv      # extract metadata to CSV
 sequelizer convert single_read_files/ --to multi-read -o combined.fast5        # single to multi
 sequelizer convert multi_read.fast5 --to single-read -o extracted              # multi to single
 sequelizer convert input.fast5 --recompress --compression gzip -o optim.fast5  # compression optimization
 sequelizer convert data/ --recursive --to slow5 --output-dir conv/             # dir conv w/ filt
 sequelizer convert data/ --to metadata --format csv --combine -o data_meta.csv # dir conv w/ filt
*/

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