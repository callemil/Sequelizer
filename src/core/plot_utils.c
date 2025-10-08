// **********************************************************************
// core/plot_utils.c - Plotting Utilities and Data Structures
// **********************************************************************
// Sebastian Claudiusz Magierowski Sep 30 2025
//

#include "plot_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// **********************************************************************
// File Format Detection
// **********************************************************************
// Auto-detect file format by examining headers and data structure (squiggle vs raw vs unknown)
file_format_t detect_plot_file_format(FILE *fp) {
  char line[1024];
  long start_pos = ftell(fp);
  bool has_channel_metadata = false;
  bool found_numeric_data = false;

  while (fgets(line, sizeof(line), fp)) {
    // Check for sequelizer convert metadata markers
    if (strstr(line, "# Channel:") || strstr(line, "# Sample Rate:") ||
        strstr(line, "# Read ID:") || strstr(line, "# Offset:")) {
      has_channel_metadata = true;
      continue;
    }

    // Skip other comments and empty lines
    if (line[0] == '#' || line[0] == '\n') {
      continue;
    }

    // Check for explicit headers
    if (strstr(line, "sample_index") || strstr(line, "Sample Index")) {
      fseek(fp, start_pos, SEEK_SET);
      return FILE_FORMAT_RAW;
    }

    if (strstr(line, "pos") && strstr(line, "base") && strstr(line, "current")) {
      fseek(fp, start_pos, SEEK_SET);
      return FILE_FORMAT_SQUIGGLE;
    }

    // Check first data line format
    if (!found_numeric_data) {
      if (strchr(line, '\t')) {
        int tab_count = 0;
        for (char *p = line; *p; p++) {
          if (*p == '\t') tab_count++;
        }

        fseek(fp, start_pos, SEEK_SET);
        if (tab_count == 1) {
          return FILE_FORMAT_RAW;       // sample_index, raw_value
        } else if (tab_count == 4) {
          return FILE_FORMAT_SQUIGGLE;  // pos, base, current, sd, dwell
        }
      }

      // Check if it's just a numeric value (sequelizer convert raw output)
      float test_value;
      if (sscanf(line, "%f", &test_value) == 1) {
        found_numeric_data = true;
        // If we found channel metadata earlier, this is likely raw format
        if (has_channel_metadata) {
          fseek(fp, start_pos, SEEK_SET);
          return FILE_FORMAT_RAW;
        }
      }
    }
  }

  fseek(fp, start_pos, SEEK_SET);

  // If we found numeric data, assume raw format as fallback
  if (found_numeric_data) {
    return FILE_FORMAT_RAW;
  }

  return FILE_FORMAT_UNKNOWN;
}

// **********************************************************************
// Data Parsing
// **********************************************************************
// Read text file line-by-line, converts text to raw_data_t in mem, handles multiple formats, returns array of raw_data_t structs
// can handle: 2-col tab-separated (0\t356\n1\t260\n2\t258), 2-col space-separted, 1-col w/ auto-indexing (356\n260\n258)
int parse_raw_file(FILE *fp, raw_data_t **out_data) {
  char line[1024];
  int data_count = 0;
  int capacity = 10000;
  raw_data_t *data = malloc(capacity * sizeof(raw_data_t));

  if (!data) {
    fprintf(stderr, "Memory allocation failed\n");
    return -1;
  }

  while (fgets(line, sizeof(line), fp)) {
    // Skip comments and headers
    if (line[0] == '#' || strstr(line, "Channel:") ||
        strstr(line, "Sample Rate:") || strstr(line, "Read ID:") ||
        strstr(line, "sample_index") || strstr(line, "Sample Index")) {
      continue;
    }

    // Skip empty lines
    if (line[0] == '\n') {
      continue;
    }

    // Try to parse as tab-separated values first
    if (sscanf(line, "%d\t%f", &data[data_count].sample_index, &data[data_count].raw_value) == 2) {
      data_count++;
    }
    // If tab-separated fails, try space-separated or just raw values
    else if (sscanf(line, "%d %f", &data[data_count].sample_index, &data[data_count].raw_value) == 2) {
      data_count++;
    }
    // If no index, assume sequential indexing (just raw values)
    else if (sscanf(line, "%f", &data[data_count].raw_value) == 1) {
      data[data_count].sample_index = data_count;
      data_count++;
    }

    // Resize array if needed
    if (data_count >= capacity) {
      capacity *= 2;
      data = realloc(data, capacity * sizeof(raw_data_t));
      if (!data) {
        fprintf(stderr, "Memory reallocation failed\n");
        return -1;
      }
    }
  }

  *out_data = data;
  return data_count;
}

// **********************************************************************
// Main Plotting Function
// **********************************************************************
// Coordinator: takes list of CL files, loops through them, detects format, opens file, calls parse_raw_file() & plot callback
int plot_signals(char **files, int file_count, const char *output_file, bool verbose,
                 int (*plot_callback)(raw_data_t *data, int count, const char *title)) {
  int total_data_points = 0;

  if (verbose) {
    printf("Processing %d files for plotting...\n", file_count);
  }

  for (int fn = 0; fn < file_count; fn++) {
    FILE *fh = fopen(files[fn], "r");
    if (!fh) {
      fprintf(stderr, "Failed to open \"%s\" for input.\n", files[fn]);
      continue;
    }

    if (verbose) {
      printf("Processing file: %s\n", files[fn]);
    }

    // Detect file format using shared utility
    file_format_t format = detect_plot_file_format(fh);
    int data_count = 0;

    switch (format) {
      case FILE_FORMAT_RAW: {
        if (verbose) {
          printf("  -> Detected raw signal format\n");
        }

        raw_data_t *raw_data = NULL;
        data_count = parse_raw_file(fh, &raw_data);

        if (data_count > 0) {
          if (verbose) {
            printf("  -> Parsed %d raw signal points\n", data_count);
            printf("  -> Creating raw signal plot...\n");
          }
          if (plot_callback) {
            plot_callback(raw_data, data_count, files[fn]);
          }
        } else {
          printf("  -> No valid data found in file\n");
        }

        free(raw_data);
        break;
      }

      case FILE_FORMAT_SQUIGGLE:
        printf("  -> Detected squiggle format (not yet implemented)\n");
        break;

      case FILE_FORMAT_UNKNOWN:
      default:
        printf("  -> Unknown file format, skipping\n");
        break;
    }

    total_data_points += data_count;
    fclose(fh);
  }

  if (verbose) {
    printf("Processed %d files with %d total data points.\n", file_count, total_data_points);
  }

  return 0; // Changed from EXIT_SUCCESS to 0 for consistency
}

