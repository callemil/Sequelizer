// **********************************************************************
// core/fast5_stats.c
// **********************************************************************
// S Magierowski Aug 14 2025

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

// Signal statistics calculation (extracted from STEP 7B)
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