// **********************************************************************
// core/fast5_stats.h
// **********************************************************************
// S Magierowski Aug 14 2025

#ifndef SEQUELIZER_FAST5_STATS_H
#define SEQUELIZER_FAST5_STATS_H

#include "../../include/sequelizer.h"
#include "fast5_utils.h"  // fast5_metadata_t fast5_analysis_summary_t

// Sensor analysis structure
typedef struct {
  char *channel_number;      // Sensor identifier
  int read_count;           // Reads from this sensor
  char *experiment_id;      // Which experiment this belongs to
} sensor_summary_t;

// Enhanced experiment summary
typedef struct {
  char *run_id;              // Experiment identifier
  int file_count;            // Files in this experiment  
  int total_reads;           // Total reads across all files
  char **file_paths;         // Which files belong to this experiment
  
  // Stage 2 additions: Sensor analysis
  sensor_summary_t *sensors; // Array of sensors in this experiment
  int sensor_count;          // Number of unique sensors
  double avg_reads_per_sensor;
  int min_reads_per_sensor;
  int max_reads_per_sensor;
  char *most_productive_sensor; // Channel number of most productive sensor
  
  // Stage 2 additions: Experimental duration
  uint64_t min_start_time;   // Earliest read start time (samples)
  uint64_t max_start_time;   // Latest read start time (samples) 
  double duration_seconds;   // Experimental duration in seconds
  double duration_minutes;   // Experimental duration in minutes
  double avg_sample_rate;    // Average sample rate for duration calculation
  
  // Stage 3 additions: Throughput analysis
  double reads_per_sensor_per_minute;    // Average throughput per sensor
  double total_reads_per_minute;         // Total data acquisition rate  
  double sensor_efficiency_score;        // Relative productivity (0-1 scale)
} experiment_summary_t;

// Comprehensive Fast5 dataset statistics structure
typedef struct {
  // Signal statistics
  int successful_files;
  int total_reads;
  uint64_t total_samples;
  uint32_t min_signal_length;
  uint32_t max_signal_length;
  double total_duration_seconds;
  double total_file_size_mb;
  
  // Compression statistics  
  double total_compression_ratio;
  double total_effective_bits;
  int files_with_compression_stats;
  
  // Pore level statistics
  double total_median_before;
  int files_with_pore_level_stats;
  
  // Sample rate frequency statistics
  double avg_sampling_rate;
  double min_sampling_rate; 
  double max_sampling_rate;
  int files_with_rate_variation;        // Files containing multiple sampling rates
  int total_rate_variations;            // Total number of different rates found
  bool has_uniform_rates;               // True if all reads have same rate
  
  // Temporal analysis (Stage 1: Experiment discovery)
  experiment_summary_t *experiments;
  int experiment_count;
  int total_files_with_temporal_data;
  
  // Stage 2: Detailed experiment analysis
  double total_experimental_time_minutes; // Sum of all experiment durations
  int experiments_with_sensor_data;       // Experiments with complete sensor info
  
  // Stage 3: Global throughput statistics
  double global_reads_per_minute;        // Dataset-wide acquisition rate
  double avg_reads_per_sensor_per_minute; // Average across all experiments
  double peak_throughput;                // Highest reads/sensor/minute found
  char *peak_throughput_experiment;      // run_id of most productive experiment
  int experiments_with_throughput_data;  // Experiments with valid duration > 0
  
  // Future statistics can be added here
} fast5_dataset_statistics_t;


// enhancer function template for stats calculation
typedef void (*stats_enhancer_t)(fast5_dataset_statistics_t *stats, fast5_metadata_t **results, 
                                 int *results_count, 
                                 char **filenames, 
                                 size_t file_count);

// enhancer function template for summary calculation
typedef void (*summary_enhancer_t)(fast5_analysis_summary_t *summary, fast5_dataset_statistics_t *stats);

// Main stats calculation function
fast5_dataset_statistics_t* calc_fast5_dataset_stats_with_enhancer(fast5_metadata_t **results, 
                                                            int *results_count, 
                                                            char **filenames,
                                                            size_t file_count, stats_enhancer_t enhancer);

// Individual calculation functions for modularity
void calc_signal_stats(fast5_dataset_statistics_t *stats, fast5_metadata_t **results, 
                                int *results_count, char **filenames, size_t file_count);

// Main analysis summary calculation function
fast5_analysis_summary_t* calc_analysis_summary_with_enhancer(fast5_dataset_statistics_t *stats, int file_count, double processing_time_ms, summary_enhancer_t enhancer);

#endif //SEQUELIZER_FAST5_STATS_H
