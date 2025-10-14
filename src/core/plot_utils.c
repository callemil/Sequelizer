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
  // ========================================================================
  // STEP 1: INITIALIZE DETECTION STATE
  // ========================================================================
  char line[1024];
  long start_pos = ftell(fp);
  bool has_channel_metadata = false;
  bool found_numeric_data = false;

  // ========================================================================
  // STEP 2: SCAN FILE LINE-BY-LINE FOR FORMAT INDICATORS
  // ========================================================================
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

    // ========================================================================
    // STEP 3: CHECK FOR EXPLICIT COLUMN HEADERS
    // ========================================================================
    if (strstr(line, "sample_index") || strstr(line, "Sample Index")) {
      fseek(fp, start_pos, SEEK_SET);
      return FILE_FORMAT_RAW;
    }

    if (strstr(line, "pos") && strstr(line, "base") && strstr(line, "current")) {
      fseek(fp, start_pos, SEEK_SET);
      return FILE_FORMAT_SQUIGGLE;
    }

    // ========================================================================
    // STEP 4: ANALYZE FIRST DATA LINE STRUCTURE (TAB COUNT)
    // ========================================================================
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

      // ========================================================================
      // STEP 5: CHECK FOR SIMPLE NUMERIC FORMAT (SINGLE COLUMN)
      // ========================================================================
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

  // ========================================================================
  // STEP 6: RESTORE FILE POSITION AND RETURN BEST GUESS
  // ========================================================================
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
  // ========================================================================
  // STEP 1: ALLOCATE INITIAL DATA ARRAY WITH DYNAMIC CAPACITY
  // ========================================================================
  char line[1024];
  int data_count = 0;
  int capacity = 10000;
  raw_data_t *data = malloc(capacity * sizeof(raw_data_t));

  if (!data) {
    fprintf(stderr, "Memory allocation failed\n");
    return -1;
  }

  // ========================================================================
  // STEP 2: READ FILE LINE-BY-LINE AND PARSE NUMERIC DATA
  // ========================================================================
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

    // ========================================================================
    // STEP 3: TRY MULTIPLE PARSING STRATEGIES (TAB, SPACE, SINGLE-COLUMN)
    // ========================================================================
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

    // ========================================================================
    // STEP 4: DYNAMICALLY RESIZE ARRAY IF CAPACITY IS EXCEEDED
    // ========================================================================
    if (data_count >= capacity) {
      capacity *= 2;
      data = realloc(data, capacity * sizeof(raw_data_t));
      if (!data) {
        fprintf(stderr, "Memory reallocation failed\n");
        return -1;
      }
    }
  }

  // ========================================================================
  // STEP 5: RETURN PARSED DATA ARRAY AND COUNT
  // ========================================================================
  *out_data = data;
  return data_count;
}

// Parse squiggle format file (pos base current sd dwell) into memory structure
int parse_squiggle_file(FILE *fp, squiggle_data_t **out_data) {
  // ========================================================================
  // STEP 1: ALLOCATE INITIAL DATA ARRAY WITH DYNAMIC CAPACITY
  // ========================================================================
  char line[1024];
  int data_count = 0;
  int capacity = 1000;
  squiggle_data_t *data = malloc(capacity * sizeof(squiggle_data_t));

  if (!data) {
    fprintf(stderr, "Memory allocation failed\n");
    return -1;
  }

  // ========================================================================
  // STEP 2: READ FILE LINE-BY-LINE AND PARSE SQUIGGLE DATA
  // ========================================================================
  while (fgets(line, sizeof(line), fp)) {
    // Skip console output and comments
    if (line[0] == '\n' || strncmp(line, "Executing", 9) == 0 ||
        strstr(line, ": ") || strncmp(line, "Processed", 9) == 0 ||
        line[0] == '#') {
      continue;
    }

    // Skip header line
    if (strstr(line, "pos\tbase\tcurrent") || strstr(line, "pos") && strstr(line, "base")) {
      continue;
    }

    // ========================================================================
    // STEP 3: PARSE SQUIGGLE DATA LINE (pos, base, current, sd, dwell)
    // ========================================================================
    if (sscanf(line, "%d\t%c\t%f\t%f\t%f",
               &data[data_count].pos, &data[data_count].base,
               &data[data_count].current, &data[data_count].sd,
               &data[data_count].dwell) == 5) {

      data_count++;

      // ========================================================================
      // STEP 4: DYNAMICALLY RESIZE ARRAY IF CAPACITY IS EXCEEDED
      // ========================================================================
      if (data_count >= capacity) {
        capacity *= 2;
        data = realloc(data, capacity * sizeof(squiggle_data_t));
        if (!data) {
          fprintf(stderr, "Memory reallocation failed\n");
          return -1;
        }
      }
    }
  }

  // ========================================================================
  // STEP 5: RETURN PARSED DATA ARRAY AND COUNT
  // ========================================================================
  *out_data = data;
  return data_count;
}

// **********************************************************************
// Main Plotting Function
// **********************************************************************
// Coordinator: takes list of CL files, loops through them, detects format, opens file, calls parse functions & callbacks
// Uses struct-based parameters for scalability - add new options to structs, not function signature
int plot_signals(char **files, int file_count, plot_config_t *config, plot_callbacks_t *callbacks) {
  // ========================================================================
  // STEP 1: INITIALIZE TRACKING VARIABLES
  // ========================================================================
  int total_data_points = 0;

  if (config->verbose) {
    printf("Processing %d files for plotting...\n", file_count);
  }

  // ========================================================================
  // STEP 2: ITERATE THROUGH ALL INPUT FILES
  // ========================================================================
  for (int fn = 0; fn < file_count; fn++) {
    FILE *fh = fopen(files[fn], "r");
    if (!fh) {
      fprintf(stderr, "Failed to open \"%s\" for input.\n", files[fn]);
      continue;
    }

    if (config->verbose) {
      printf("Processing file: %s\n", files[fn]);
    }

    // ========================================================================
    // STEP 3: AUTO-DETECT FILE FORMAT (RAW, SQUIGGLE, OR UNKNOWN)
    // ========================================================================
    file_format_t format = detect_plot_file_format(fh);
    int data_count = 0;

    // ========================================================================
    // STEP 4: PROCESS FILE BASED ON DETECTED FORMAT
    // ========================================================================
    switch (format) {
      case FILE_FORMAT_RAW: {
        if (config->verbose) {
          printf("  -> Detected raw signal format\n");
        }

        raw_data_t *raw_data = NULL;
        data_count = parse_raw_file(fh, &raw_data);

        if (data_count > 0) {
          if (config->verbose) {
            printf("  -> Parsed %d raw signal points\n", data_count);
          }
          // ========================================================================
          // STEP 4.1: INVOKE RAW PLOTTING CALLBACK WITH PARSED DATA
          // ========================================================================
          // Choose callback based on PNG mode
          if (config->png_mode) {
            // PNG mode - generate filename and use PNG callback
            if (callbacks && callbacks->plot_raw_png) {
              char png_filename[256];
              snprintf(png_filename, sizeof(png_filename), "%s_raw.png", files[fn]);
              if (config->verbose) {
                printf("  -> Creating PNG: %s\n", png_filename);
              }
              callbacks->plot_raw_png(raw_data, data_count, png_filename);
            } else if (config->verbose) {
              printf("  -> Warning: No raw PNG callback provided\n");
            }
          } else {
            // Interactive mode - use interactive callback
            if (callbacks && callbacks->plot_raw) {
              if (config->verbose) {
                printf("  -> Creating interactive plot...\n");
              }
              // Use provided title or fallback to filename
              const char *plot_title = config->title ? config->title : files[fn];
              callbacks->plot_raw(raw_data, data_count, plot_title);
            } else if (config->verbose) {
              printf("  -> Warning: No raw plotting callback provided\n");
            }
          }
        } else {
          printf("  -> No valid data found in file\n");
        }

        free(raw_data);
        break;
      }

      case FILE_FORMAT_SQUIGGLE: {
        if (config->verbose) {
          printf("  -> Detected squiggle format\n");
        }

        squiggle_data_t *squiggle_data = NULL;
        data_count = parse_squiggle_file(fh, &squiggle_data);

        if (data_count > 0) {
          if (config->verbose) {
            printf("  -> Parsed %d squiggle data points\n", data_count);
          }
          // ========================================================================
          // STEP 4.2: INVOKE SQUIGGLE PLOTTING CALLBACK WITH PARSED DATA
          // ========================================================================
          // Choose callback based on PNG mode
          if (config->png_mode) {
            // PNG mode - generate filename and use PNG callback
            if (callbacks && callbacks->plot_squiggle_png) {
              char png_filename[256];
              snprintf(png_filename, sizeof(png_filename), "%s_squiggle.png", files[fn]);
              if (config->verbose) {
                printf("  -> Creating PNG: %s\n", png_filename);
              }
              callbacks->plot_squiggle_png(squiggle_data, data_count, png_filename);
            } else if (config->verbose) {
              printf("  -> Warning: No squiggle PNG callback provided\n");
            }
          } else {
            // Interactive mode - use interactive callback
            if (callbacks && callbacks->plot_squiggle) {
              if (config->verbose) {
                printf("  -> Creating interactive plot...\n");
              }
              // Use provided title or fallback to filename
              const char *plot_title = config->title ? config->title : files[fn];
              callbacks->plot_squiggle(squiggle_data, data_count, plot_title);
            } else if (config->verbose) {
              printf("  -> Warning: No squiggle plotting callback provided\n");
            }
          }
        } else {
          printf("  -> No valid data found in file\n");
        }

        free(squiggle_data);
        break;
      }

      case FILE_FORMAT_UNKNOWN:
      default:
        printf("  -> Unknown file format, skipping\n");
        break;
    }

    // ========================================================================
    // STEP 6: ACCUMULATE STATISTICS AND CLEAN UP FILE HANDLE
    // ========================================================================
    total_data_points += data_count;
    fclose(fh);
  }

  // ========================================================================
  // STEP 7: REPORT FINAL STATISTICS
  // ========================================================================
  if (config->verbose) {
    printf("Processed %d files with %d total data points.\n", file_count, total_data_points);
  }

  return 0; // Changed from EXIT_SUCCESS to 0 for consistency
}

