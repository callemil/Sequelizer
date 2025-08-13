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

// Forward declaration for detailed experiment summary (defined in ciren_fast5_stats.h)
// Note: This will be properly defined when ciren_fast5_stats.h is included

// Comprehensive analysis summary (shared by both ciren and sequelizer)
typedef struct {
  int total_files;
  int successful_files;
  int failed_files;
  int total_reads;
  double processing_time_ms;
  int threads_used;
  char *command_line;
  // Extended statistics
  double total_file_size_mb;
  uint64_t total_samples;
  uint32_t min_signal_length;
  uint32_t max_signal_length;
  double avg_signal_length;
  double total_duration_seconds;
  double avg_duration_seconds;
  double avg_bits_per_sample;
  // Compression statistics
  double avg_compression_ratio;
  double avg_effective_bits_per_sample;
  int files_with_compression_stats;
  // Pore level statistics
  double avg_median_before;
  int files_with_pore_level_stats;
  // Sample rate frequency statistics
  double avg_sampling_rate;
  double min_sampling_rate;
  double max_sampling_rate;
  int files_with_rate_variation;
  bool has_uniform_rates;
  // Temporal analysis statistics
  int experiment_count;
  int total_files_with_temporal_data;
  double total_experimental_time_minutes;
  int experiments_with_sensor_data;
  void *experiments;  // Pointer to experiment data (experiment_summary_t* in ciren)
  // Stage 3: Throughput statistics
  double global_reads_per_minute;
  double avg_reads_per_sensor_per_minute;
  double peak_throughput;
  char *peak_throughput_experiment;
  int experiments_with_throughput_data;
} fast5_analysis_summary_t;

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

// Comprehensive summary calculation and output (for shared use)
fast5_analysis_summary_t* calculate_comprehensive_summary(void **results, 
                                                         char **filenames, 
                                                         int *results_count,
                                                         int file_count,
                                                         double processing_time_ms,
                                                         int threads_used,
                                                         char *command_line);

// Comprehensive summary output functions (for shared use)
void print_comprehensive_summary_human(fast5_analysis_summary_t *summary);

// Memory cleanup
void free_basic_summary(basic_fast5_summary_t *summary);
void free_comprehensive_summary(fast5_analysis_summary_t *summary);

#endif // SEQUELIZER_FAST5_UTILS_H