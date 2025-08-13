// **********************************************************************
// core/fast5_utils.c - Shared Fast5 Utility Functions
// **********************************************************************
// S Magierowski Aug 3 2025
// 
// Basic utilities shared between ciren and sequelizer for consistent
// file size calculations and basic summary functionality.
//
// This module provides only basic functionality:
// - Decimal MB file size calculation (1000^2)
// - Basic summary structure and calculation
// - Simple human-readable output
//
// Advanced features (compression analysis, JSON, threading) remain
// in ciren-specific code for feature differentiation.

#include "fast5_utils.h"
#include "fast5_io.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdint.h>

// **********************************************************************
// File Size Utility
// **********************************************************************

// Calculate file size in decimal MB (1000^2) to match OS file managers
double get_file_size_mb(const char *filename) {
  struct stat file_stat;
  if (stat(filename, &file_stat) != 0) {
    return 0.0;
  }
  return (double)file_stat.st_size / (1000.0 * 1000.0);
}

// **********************************************************************
// Basic Summary Functions
// **********************************************************************

// Calculate basic summary statistics from Fast5 processing results
basic_fast5_summary_t* calculate_basic_summary(void **results, 
                                              char **filenames, 
                                              int *results_count,
                                              int file_count,
                                              double processing_time_ms) {
  if (!results || !filenames || !results_count) {
    return NULL;
  }
  
  // Cast void** back to fast5_metadata_t** for use
  fast5_metadata_t **metadata_results = (fast5_metadata_t**)results;
  
  basic_fast5_summary_t *summary = malloc(sizeof(basic_fast5_summary_t));
  if (!summary) {
    return NULL;
  }
  
  // Initialize counters
  summary->total_files = file_count;
  summary->successful_files = 0;
  summary->failed_files = 0;
  summary->total_reads = 0;
  summary->total_file_size_mb = 0.0;
  summary->total_samples = 0;
  summary->avg_signal_length = 0.0;
  summary->processing_time_ms = processing_time_ms;
  
  // Calculate statistics from results
  for (int i = 0; i < file_count; i++) {
    if (metadata_results[i] && results_count[i] > 0) {
      // Successful file
      summary->successful_files++;
      summary->total_reads += results_count[i];
      summary->total_file_size_mb += get_file_size_mb(filenames[i]);
      
      // Sum signal lengths for average calculation
      for (int j = 0; j < results_count[i]; j++) {
        summary->total_samples += metadata_results[i][j].signal_length;
      }
    } else {
      // Failed file
      summary->failed_files++;
    }
  }
  
  // Calculate average signal length
  if (summary->total_reads > 0) {
    summary->avg_signal_length = (double)summary->total_samples / summary->total_reads;
  }
  
  return summary;
}

// **********************************************************************
// Basic Human-Readable Output
// **********************************************************************

// Print basic summary in human-readable format (no JSON, no compression stats)
void print_basic_summary_human(basic_fast5_summary_t *summary) {
  if (!summary) {
    return;
  }
  
  printf("Sequelizer Fast5 Dataset Analysis Summary\n");
  printf("=========================================\n");
  printf("Files processed: %d/%d successful", summary->successful_files, summary->total_files);
  if (summary->failed_files > 0) {
    printf(" (%d failed)", summary->failed_files);
  }
  printf("\n");
  
  printf("Total file size: %.1f MB\n", summary->total_file_size_mb);
  printf("Total reads: %d\n", summary->total_reads);
  
  if (summary->total_reads > 0) {
    printf("Signal statistics:\n");
    printf("  Total samples: %llu\n", (unsigned long long)summary->total_samples);
    printf("  Average length: %.0f samples\n", summary->avg_signal_length);
  }
  
  if (summary->processing_time_ms > 0) {
    printf("Processing time: %.2f seconds\n", summary->processing_time_ms / 1000.0);
  }
  printf("\n");
}

// **********************************************************************
// Comprehensive Summary Functions (for shared use by ciren and sequelizer)
// **********************************************************************

// Calculate comprehensive summary with basic statistics (sequelizer level)
fast5_analysis_summary_t* calculate_comprehensive_summary(void **results, 
                                                         char **filenames, 
                                                         int *results_count,
                                                         int file_count,
                                                         double processing_time_ms,
                                                         int threads_used,
                                                         char *command_line) {
  fast5_analysis_summary_t *summary = calloc(1, sizeof(fast5_analysis_summary_t));
  if (!summary) return NULL;
  
  // Initialize basic fields
  summary->total_files = file_count;
  summary->processing_time_ms = processing_time_ms;
  summary->threads_used = threads_used;
  summary->command_line = command_line ? strdup(command_line) : NULL;
  
  // Calculate basic statistics
  int successful_files = 0;
  int total_reads = 0;
  uint64_t total_samples = 0;
  double total_file_size_mb = 0.0;
  uint32_t min_signal_length = UINT32_MAX;
  uint32_t max_signal_length = 0;
  
  for (int i = 0; i < file_count; i++) {
    if (results_count[i] > 0) {
      successful_files++;
      total_reads += results_count[i];
      
      // Calculate file size
      if (filenames && filenames[i]) {
        total_file_size_mb += get_file_size_mb(filenames[i]);
      }
      
      // Process metadata if available
      fast5_metadata_t *metadata = (fast5_metadata_t*)results[i];
      if (metadata) {
        for (int j = 0; j < results_count[i]; j++) {
          total_samples += metadata[j].signal_length;
          if (metadata[j].signal_length > 0) {
            if (metadata[j].signal_length < min_signal_length) {
              min_signal_length = metadata[j].signal_length;
            }
            if (metadata[j].signal_length > max_signal_length) {
              max_signal_length = metadata[j].signal_length;
            }
          }
        }
      }
    }
  }
  
  // Set calculated values
  summary->successful_files = successful_files;
  summary->failed_files = file_count - successful_files;
  summary->total_reads = total_reads;
  summary->total_file_size_mb = total_file_size_mb;
  summary->total_samples = total_samples;
  summary->min_signal_length = (min_signal_length == UINT32_MAX) ? 0 : min_signal_length;
  summary->max_signal_length = max_signal_length;
  summary->avg_signal_length = total_reads > 0 ? (double)total_samples / total_reads : 0.0;
  
  // Initialize advanced fields to defaults (can be enhanced later)
  summary->total_duration_seconds = 0.0;
  summary->avg_duration_seconds = 0.0;
  summary->avg_bits_per_sample = total_samples > 0 ? (total_file_size_mb * 1000.0 * 1000.0 * 8.0) / total_samples : 0.0;
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
  
  return summary;
}

// Print comprehensive summary in human-readable format (basic version for sequelizer)
void print_comprehensive_summary_human(fast5_analysis_summary_t *summary) {
  if (!summary) return;
  
  printf("Sequelizer Fast5 Dataset Analysis Summary\n");
  printf("=========================================\n");
  printf("Files processed: %d/%d successful", summary->successful_files, summary->total_files);
  if (summary->failed_files > 0) {
    printf(" (%d failed)", summary->failed_files);
  }
  printf("\n");
  
  printf("Total file size: %.1f MB\n", summary->total_file_size_mb);
  printf("Total reads: %d\n", summary->total_reads);
  
  if (summary->total_reads > 0) {
    printf("Signal statistics:\n");
    printf("  Total samples: %llu\n", (unsigned long long)summary->total_samples);
    printf("  Average length: %.0f samples\n", summary->avg_signal_length);
    if (summary->min_signal_length > 0 && summary->max_signal_length > 0) {
      printf("  Range: %u - %u samples\n", summary->min_signal_length, summary->max_signal_length);
    }
    if (summary->avg_bits_per_sample > 0) {
      printf("  Average bits per sample: %.2f\n", summary->avg_bits_per_sample);
    }
  }
  
  if (summary->processing_time_ms > 0) {
    printf("Processing time: %.2f seconds", summary->processing_time_ms / 1000.0);
    if (summary->threads_used > 1) {
      printf(" (%d threads)", summary->threads_used);
    }
    printf("\n");
  }
  printf("\n");
}

// **********************************************************************
// Memory Management
// **********************************************************************

// Free basic summary structure
void free_basic_summary(basic_fast5_summary_t *summary) {
  if (summary) {
    free(summary);
  }
}

// Free comprehensive summary memory
void free_comprehensive_summary(fast5_analysis_summary_t *summary) {
  if (!summary) return;
  
  if (summary->command_line) {
    free(summary->command_line);
  }
  if (summary->peak_throughput_experiment) {
    free(summary->peak_throughput_experiment);
  }
  // Note: experiments pointer is not freed here as it may be managed elsewhere
  
  free(summary);
}