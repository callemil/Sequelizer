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
  
  printf("Fast5 Dataset Summary\n");
  printf("====================\n");
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
// Memory Management
// **********************************************************************

// Free basic summary structure
void free_basic_summary(basic_fast5_summary_t *summary) {
  if (summary) {
    free(summary);
  }
}