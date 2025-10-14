// **********************************************************************
// core/plot_utils.h - Plotting Utilities and Data Structures
// **********************************************************************
// Sebastian Claudiusz Magierowski Sep 30 2025
//

#ifndef PLOT_UTILS_H
#define PLOT_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// **********************************************************************
// Data Structures
// **********************************************************************

// File format detection for plotting data
typedef enum {
  FILE_FORMAT_UNKNOWN,
  FILE_FORMAT_RAW,
  FILE_FORMAT_SQUIGGLE
} file_format_t;

// Raw signal data structure (sample_index, raw_sample)
typedef struct {
  int sample_index;
  float raw_value;
} raw_data_t;

// Squiggle data structure (pos, base, current, std_dev, dwell)
typedef struct {
  int pos;
  char base;
  float current;
  float sd;
  float dwell;
} squiggle_data_t;

// **********************************************************************
// Callback Structures for Extensible Plotting
// **********************************************************************
// This struct pattern allows easy addition of new plotting capabilities
// (e.g., PNG export, direct Fast5 plotting, multi-panel plots) without
// changing core infrastructure. Simply add new callback function pointers
// as fields, set them in the caller, and check for NULL before invoking.
typedef struct {
  int (*plot_raw)(raw_data_t *data, int count, const char *title);
  int (*plot_squiggle)(squiggle_data_t *data, int count, const char *title);
  // Future callbacks can be added here:
  // int (*plot_png)(raw_data_t *data, int count, const char *filename);
  // int (*plot_fast5)(const char *fast5_file, const char *read_id);
} plot_callbacks_t;

// **********************************************************************
// Utility Functions
// **********************************************************************

// File format detection
file_format_t detect_plot_file_format(FILE *fp);

// Data parsing
int parse_raw_file(FILE *fp, raw_data_t **out_data);

// Main plotting function (takes callback struct for extensible plotting)
int plot_signals(char **files, int file_count, const char *output_file, bool verbose,
                 plot_callbacks_t *callbacks);

#endif // PLOT_UTILS_H