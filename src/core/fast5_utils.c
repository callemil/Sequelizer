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
    if (summary->total_duration_seconds > 0) {
      printf("  Total duration: %.1f seconds\n", summary->total_duration_seconds);
      printf("  Avg duration: %.1f seconds\n", summary->avg_duration_seconds);
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