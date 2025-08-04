#ifndef SEQUELIZER_FAST5_UTILS_H
#define SEQUELIZER_FAST5_UTILS_H

#include <stdint.h>

// Note: fast5_metadata_t is defined in fast5_io.h which must be included 
// by the .c file using these utilities

// **********************************************************************
// Basic Fast5 Summary Structure
// **********************************************************************

// Basic summary for simple reporting (no compression analysis)
typedef struct {
    int total_files;
    int successful_files; 
    int failed_files;
    int total_reads;
    double total_file_size_mb;
    uint64_t total_samples;
    double avg_signal_length;
    double processing_time_ms;
} basic_fast5_summary_t;

// **********************************************************************
// Utility Functions
// **********************************************************************

// File size calculation using decimal MB (1000^2)
double get_file_size_mb(const char *filename);

// Basic summary calculation and output
// Note: results parameter should be fast5_metadata_t** but using void** to avoid header conflicts
basic_fast5_summary_t* calculate_basic_summary(void **results, 
                                              char **filenames, 
                                              int *results_count,
                                              int file_count,
                                              double processing_time_ms);

// Basic human-readable summary output (no JSON, no compression stats)
void print_basic_summary_human(basic_fast5_summary_t *summary);

// Memory cleanup
void free_basic_summary(basic_fast5_summary_t *summary);

#endif // SEQUELIZER_FAST5_UTILS_H