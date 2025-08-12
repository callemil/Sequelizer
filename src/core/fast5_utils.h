// **********************************************************************
// core/fast5_utils.h - Shared Fast5 Utility Functions
// **********************************************************************
// S Magierowski Aug 3 2025
// 
#ifndef SEQUELIZER_FAST5_UTILS_H
#define SEQUELIZER_FAST5_UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  // size_t

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

// Fast5 metadata structure
typedef struct {
    char *read_id;
    uint32_t signal_length;
    double sample_rate;
    uint32_t duration;
    uint32_t read_number;
    bool is_multi_read;
    char *file_path;
    // Storage analysis fields
    char *compression_method;      // "deflate-gzip", "szip", "none"
    size_t logical_bytes;          // Uncompressed size of signal data  
    size_t datatype_size;          // Bytes per sample (2 for int16)
    double logical_bits_per_sample; // logical_bytes * 8 / signal_length
    double compression_ratio;      // If compression detected
    bool storage_analysis_available; // Whether storage analysis was performed
    // Pore level analysis fields  
    double median_before;              // Estimated median current level before read
    bool pore_level_available;         // Whether pore level analysis was performed
    // Temporal analysis fields  
    uint64_t start_time;        // Raw start time in samples
    char *run_id;              // Experiment identifier
    char *channel_number;       // Sensor/channel identifier ("1", "142", etc.)
    bool temporal_data_available; // Whether temporal analysis was performed
} fast5_metadata_t;

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