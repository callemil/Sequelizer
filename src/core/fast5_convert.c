// **********************************************************************
// core/fast5_convert.c - Fast5 Format Conversion Functions
// **********************************************************************
// S Magierowski Aug 22 2025
//

#include "fast5_convert.h"
#include "fast5_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <err.h>

// **********************************************************************
// Utility Functions
// **********************************************************************

int write_signal_to_file(const char *filename, float *signal, size_t signal_length) {
  FILE *f = fopen(filename, "w");
  if (!f) {
    warnx("Cannot create output file: %s", filename);
    return EXIT_FAILURE;
  }
  
  for (size_t i = 0; i < signal_length; i++) {
    fprintf(f, "%.6f\n", signal[i]);
  }
  
  fclose(f);
  return EXIT_SUCCESS;
}

int create_directory(const char *path) {
  struct stat st = {0};
  
  // Check if directory already exists
  if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
    return EXIT_SUCCESS;
  }
  
  // Try to create directory
  if (mkdir(path, 0755) != 0) {
    warnx("Cannot create directory: %s", path);
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}

// **********************************************************************
// Signal Extraction Functions
// **********************************************************************

int extract_raw_signals(char **files, size_t file_count, const char *output_file, 
                         bool all_reads, bool verbose) {
  if (verbose) {
    printf("Converting %zu files to raw format...\n", file_count);
  }
  
  for (size_t i = 0; i < file_count; i++) {
    if (verbose) {
      printf("Processing file: %s\n", files[i]);
    }
    
    // Read metadata to determine format and get read IDs
    size_t metadata_count = 0;
    fast5_metadata_t *metadata = read_fast5_metadata_with_enhancer(files[i], &metadata_count, NULL);
    
    if (!metadata || metadata_count == 0) {
      warnx("Cannot read metadata from file: %s", files[i]);
      continue;
    }
    
    // Handle single-read vs multi-read files
    bool is_multi_read = metadata[0].is_multi_read;
    size_t reads_to_process = metadata_count;
    
    // For multi-read files, limit to first 3 reads unless --all specified
    if (is_multi_read && !all_reads && metadata_count > 3) {
      reads_to_process = 3;
      if (verbose) {
        printf("  Multi-read file: processing first 3 of %zu reads (use --all for all)\n", metadata_count);
      }
    }
    
    if (is_multi_read && file_count == 1 && output_file) {
      // Single multi-read file with output directory
      if (create_directory(output_file) != EXIT_SUCCESS) {
        free_fast5_metadata(metadata, metadata_count);
        continue;
      }
    }
    
    // Extract signals
    for (size_t j = 0; j < reads_to_process; j++) {
      size_t signal_length = 0;
      float *signal = read_fast5_signal(files[i], metadata[j].read_id, &signal_length);
      
      if (!signal || signal_length == 0) {
        if (verbose) {
          printf("  Failed to extract signal for read: %s\n", 
                 metadata[j].read_id ? metadata[j].read_id : "unknown");
        }
        continue;
      }
      
      // Determine output filename
      char output_filename[512];
      if (is_multi_read) {
        if (file_count == 1 && output_file) {
          // Single file to directory: output_dir/read_001.txt
          snprintf(output_filename, sizeof(output_filename), "%s/read_%03zu.txt", output_file, j + 1);
        } else {
          // Multiple files or no output specified: basename_read_001.txt
          const char *basename = strrchr(files[i], '/');
          basename = basename ? basename + 1 : files[i];
          snprintf(output_filename, sizeof(output_filename), "%.*s_read_%03zu.txt", 
                   (int)(strstr(basename, ".fast5") - basename), basename, j + 1);
        }
      } else {
        // Single-read file
        if (output_file) {
          snprintf(output_filename, sizeof(output_filename), "%s", output_file);
        } else {
          const char *basename = strrchr(files[i], '/');
          basename = basename ? basename + 1 : files[i];
          snprintf(output_filename, sizeof(output_filename), "%.*s_signal.txt", 
                   (int)(strstr(basename, ".fast5") - basename), basename);
        }
      }
      
      // Write signal to file
      if (write_signal_to_file(output_filename, signal, signal_length) == EXIT_SUCCESS) {
        if (verbose) {
          printf("  Wrote %zu samples to: %s\n", signal_length, output_filename);
        }
      }
      
      free_fast5_signal(signal);
    }
    
    free_fast5_metadata(metadata, metadata_count);
  }
  
  return EXIT_SUCCESS;
}

// **********************************************************************
// Metadata Extraction Functions
// **********************************************************************

int extract_metadata(char **files, size_t file_count, const char *output_file, bool verbose) {
  if (verbose) {
    printf("Converting %zu files to metadata format...\n", file_count);
  }
  
  FILE *output = stdout;
  if (output_file) {
    output = fopen(output_file, "w");
    if (!output) {
      warnx("Cannot create output file: %s", output_file);
      return EXIT_FAILURE;
    }
  }
  
  // Write header
  fprintf(output, "# Fast5 Metadata Export\n");
  fprintf(output, "# file_path\tread_id\tsignal_length\tsample_rate\tduration\tread_number\tis_multi_read\n");
  
  for (size_t i = 0; i < file_count; i++) {
    if (verbose) {
      printf("Processing file: %s\n", files[i]);
    }
    
    // Read metadata
    size_t metadata_count = 0;
    fast5_metadata_t *metadata = read_fast5_metadata_with_enhancer(files[i], &metadata_count, NULL);
    
    if (!metadata || metadata_count == 0) {
      warnx("Cannot read metadata from file: %s", files[i]);
      continue;
    }
    
    // Write metadata for each read
    for (size_t j = 0; j < metadata_count; j++) {
      fprintf(output, "%s\t%s\t%u\t%.0f\t%u\t%u\t%s\n",
              files[i],
              metadata[j].read_id ? metadata[j].read_id : "unknown",
              metadata[j].signal_length,
              metadata[j].sample_rate,
              metadata[j].duration,
              metadata[j].read_number,
              metadata[j].is_multi_read ? "true" : "false");
    }
    
    free_fast5_metadata(metadata, metadata_count);
  }
  
  if (output != stdout) {
    fclose(output);
  }
  
  return EXIT_SUCCESS;
}