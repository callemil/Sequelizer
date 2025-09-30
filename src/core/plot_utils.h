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
// Utility Functions
// **********************************************************************

// File format detection
file_format_t detect_plot_file_format(FILE *fp);

// Data validation
bool validate_raw_data(const raw_data_t *data, int count);
bool validate_squiggle_data(const squiggle_data_t *data, int count);

// Data conversion utilities
int convert_raw_to_current(raw_data_t *data, int count, float offset, float range, float digitisation);

#endif // PLOT_UTILS_H