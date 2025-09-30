// **********************************************************************
// core/fast5_stats.c
// **********************************************************************
// Sebastian Claudiusz Magierowski Aug 14 2025

#include "fast5_stats.h"
#include <stdlib.h> // calloc

// Main calculation function that orchestrates all statistics
fast5_dataset_statistics_t* calc_fast5_dataset_stats_with_enhancer(fast5_metadata_t **results, 
                                                            int *results_count, 
                                                            char **filenames,
                                                            size_t file_count, stats_enhancer_t enhancer) {
  RETURN_NULL_IF(NULL == results, NULL);
  RETURN_NULL_IF(NULL == results_count, NULL);
  RETURN_NULL_IF(NULL == filenames, NULL);

  // Allocate statistics structure
  fast5_dataset_statistics_t *stats = calloc(1, sizeof(fast5_dataset_statistics_t));
  RETURN_NULL_IF(NULL == stats, NULL);
  
  // Initialize min_signal_length to handle edge case
  stats->min_signal_length = UINT32_MAX;

  // sequelizer enhancer
  calc_signal_stats(stats, results, results_count, filenames, file_count);

  // *** CALL ENHANCER HERE - while signal dataset is open ***
  if (enhancer) {
    enhancer(stats, results, results_count, filenames, file_count);
  }

  // Handle edge case where no reads were found
  if (stats->total_reads == 0) {
    stats->min_signal_length = 0;
  }
  
  return stats;  
}

// Signal statistics calculation (a local enhancer)
void calc_signal_stats(fast5_dataset_statistics_t *stats, fast5_metadata_t **results, 
                                int *results_count, char **filenames, size_t file_count) {
  if (!stats || !results || !results_count || !filenames) return;
  
  // Initialize statistical counters
  stats->successful_files = 0;
  stats->total_reads = 0;
  stats->total_file_size_mb = 0.0;
  stats->total_samples = 0;
  stats->min_signal_length = UINT32_MAX;
  stats->max_signal_length = 0;
  stats->total_duration_seconds = 0.0;
  
  // Iterate through all results to calculate dataset-wide statistics
  for (size_t i = 0; i < file_count; i++) {
    if (results[i] && results_count[i] > 0) { // File processed successfully
      stats->successful_files++;
      stats->total_reads += results_count[i];
      stats->total_file_size_mb += get_file_size_mb(filenames[i]);
      
      // Calculate per-read signal statistics
      for (int j = 0; j < results_count[i]; j++) {
        uint32_t signal_length = results[i][j].signal_length;
        stats->total_samples += signal_length;
        
        // Track signal length range
        if (signal_length > 0) {
          if (signal_length < stats->min_signal_length) {
            stats->min_signal_length = signal_length;
          }
          if (signal_length > stats->max_signal_length) {
            stats->max_signal_length = signal_length;
          }
        }
        
        // Convert duration from samples to seconds using sample rate
        if (results[i][j].sample_rate > 0) {
          stats->total_duration_seconds += (double)results[i][j].duration / results[i][j].sample_rate;
        }
      }
    }
  }
}

// Calculate comprehensive summary with basic statistics (sequelizer level)
fast5_analysis_summary_t* calc_analysis_summary_with_enhancer(fast5_dataset_statistics_t *stats, int file_count, double processing_time_ms, summary_enhancer_t enhancer) {

  fast5_analysis_summary_t *summary = calloc(1, sizeof(fast5_analysis_summary_t));
  if (!summary) return NULL;

  summary->total_files            = file_count;
  summary->total_file_size_mb     = stats->total_file_size_mb;
  summary->successful_files       = stats->successful_files;
  summary->failed_files           = file_count - stats->successful_files;
  summary->total_reads            = stats->total_reads;
  summary->total_samples          = stats->total_samples;
  summary->avg_signal_length      = stats->total_reads > 0 ? (double)stats->total_samples / stats->total_reads : 0.0;
  summary->min_signal_length      = stats->min_signal_length;
  summary->max_signal_length      = stats->max_signal_length;
  summary->total_duration_seconds = stats->total_duration_seconds;
  summary->avg_bits_per_sample    = stats->total_samples > 0 ? (stats->total_file_size_mb * 1000.0 * 1000.0 * 8.0) / stats->total_samples : 0.0;
  summary->processing_time_ms     = processing_time_ms;
  summary->avg_duration_seconds   = stats->total_reads > 0 ? stats->total_duration_seconds / stats->total_reads : 0.0;

  if (enhancer) {
    enhancer(summary, stats);
  } else {
    // Initialize advanced fields to defaults (can be enhanced later)
    summary->avg_compression_ratio = 0.0;
    summary->avg_effective_bits_per_sample = 0.0;
    summary->files_with_compression_stats = 0;
    summary->avg_median_before = 0.0;
    summary->files_with_pore_level_stats = 0;
    summary->avg_sampling_rate = 0.0;
    summary->min_sampling_rate = 0.0;
    summary->max_sampling_rate = 0.0;
    summary->files_with_rate_variation = 0;
    summary->has_uniform_rates = false;
    summary->experiment_count = 0;
    summary->total_files_with_temporal_data = 0;
    summary->total_experimental_time_minutes = 0.0;
    summary->experiments_with_sensor_data = 0;
    summary->experiments = NULL;
    summary->global_reads_per_minute = 0.0;
    summary->avg_reads_per_sensor_per_minute = 0.0;
    summary->peak_throughput = 0.0;
    summary->peak_throughput_experiment = NULL;
    summary->experiments_with_throughput_data = 0;
  }
  return summary;
}