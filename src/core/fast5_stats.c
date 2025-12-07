// **********************************************************************
// core/fast5_stats.c
// **********************************************************************
// Sebastian Claudiusz Magierowski Aug 14 2025

#include "fast5_stats.h"
#include "fast5_io.h"
#include <stdlib.h> // calloc
#include <string.h>
#include <math.h>

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

// **********************************************************************
// Summary File Writing Functions
// **********************************************************************

// Comparison function for qsort (for median calculation)
static int compare_int16(const void *a, const void *b) {
  int16_t ia = *(const int16_t *)a;
  int16_t ib = *(const int16_t *)b;
  return (ia > ib) - (ia < ib);
}

// Calculate median of int16_t array
double calculate_median_int16(int16_t *data, size_t length) {
  if (length == 0) return 0.0;

  // Create a copy for sorting
  int16_t *sorted = malloc(length * sizeof(int16_t));
  if (!sorted) return 0.0;

  memcpy(sorted, data, length * sizeof(int16_t));
  qsort(sorted, length, sizeof(int16_t), compare_int16);

  double median;
  if (length % 2 == 0) {
    median = (sorted[length/2 - 1] + sorted[length/2]) / 2.0;
  } else {
    median = sorted[length/2];
  }

  free(sorted);
  return median;
}

// Calculate MAD (Median Absolute Deviation) of int16_t array
double calculate_mad_int16(int16_t *data, size_t length, double median) {
  if (length == 0) return 0.0;

  // Calculate absolute deviations
  int16_t *abs_dev = malloc(length * sizeof(int16_t));
  if (!abs_dev) return 0.0;

  for (size_t i = 0; i < length; i++) {
    abs_dev[i] = (int16_t)fabs(data[i] - median);
  }

  double mad = calculate_median_int16(abs_dev, length);
  free(abs_dev);

  return mad;
}

// Write summary header to file
void write_summary_header(FILE *fp) {
  fprintf(fp, "#sequelizer_summary_v1.0\n");
  fprintf(fp, "filename\tread_id\trun_id\tchannel\tstart_time\tmux\tduration\tnum_samples\tmedian_pa\tmad_pa\n");
}

// Write a single read's summary row
void write_summary_row(FILE *fp, const char *filename,
                      fast5_metadata_t *metadata,
                      int16_t *signal, size_t signal_length) {
  // Extract basename from filename
  const char *basename = strrchr(filename, '/');
  basename = basename ? basename + 1 : filename;

  // Calculate median and MAD from raw signal
  double median_raw = calculate_median_int16(signal, signal_length);
  double mad_raw = calculate_mad_int16(signal, signal_length, median_raw);

  // Convert to pA if calibration available
  double median_pa, mad_pa;
  if (metadata->calibration_available && metadata->digitisation > 0) {
    median_pa = (median_raw - metadata->offset) * metadata->range / metadata->digitisation;
    mad_pa = mad_raw * metadata->range / metadata->digitisation;
  } else {
    // No calibration: output raw ADC values
    median_pa = median_raw;
    mad_pa = mad_raw;
  }

  // Calculate duration and start_time in seconds
  double duration = metadata->sample_rate > 0 ? metadata->signal_length / metadata->sample_rate : 0.0;
  double start_time = metadata->sample_rate > 0 ? metadata->start_time / metadata->sample_rate : 0.0;

  // Extract mux from metadata (not currently in structure, use placeholder)
  int mux = 0;  // TODO: Extract from Fast5 if available

  // Parse channel number (currently stored as string)
  int channel = metadata->channel_number ? atoi(metadata->channel_number) : 0;

  // Write the row
  fprintf(fp, "%s\t%s\t%s\t%d\t%.6f\t%d\t%.6f\t%u\t%.2f\t%.2f\n",
          basename,
          metadata->read_id ? metadata->read_id : "unknown",
          metadata->run_id ? metadata->run_id : "unknown",
          channel,
          start_time,
          mux,
          duration,
          metadata->signal_length,
          median_pa,
          mad_pa);
}

// Write complete summary file (worker function called by enhancer)
void write_summary_file(const char *summary_path, fast5_metadata_t **results,
                       int *results_count, char **filenames, size_t file_count) {
  // Open summary file
  FILE *fp = fopen(summary_path, "w");
  if (!fp) {
    fprintf(stderr, "Warning: Failed to open summary file: %s\n", summary_path);
    return;
  }

  // Write header
  write_summary_header(fp);

  // Iterate through all files and reads
  for (size_t i = 0; i < file_count; i++) {
    if (results[i] && results_count[i] > 0) {
      for (int j = 0; j < results_count[i]; j++) {
        // Read the raw signal for this read
        size_t signal_length = 0;
        float *signal_float = read_fast5_signal(filenames[i], results[i][j].read_id, &signal_length);

        if (signal_float && signal_length > 0) {
          // Convert float signal to int16_t for median/MAD calculation
          int16_t *signal_int16 = malloc(signal_length * sizeof(int16_t));
          if (signal_int16) {
            for (size_t k = 0; k < signal_length; k++) {
              signal_int16[k] = (int16_t)signal_float[k];
            }

            // Write summary row
            write_summary_row(fp, filenames[i], &results[i][j], signal_int16, signal_length);

            free(signal_int16);
          }
          free_fast5_signal(signal_float);
        }
      }
    }
  }

  fclose(fp);
  printf("Summary written to: %s\n", summary_path);
}