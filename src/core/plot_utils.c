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
// Data Validation
// **********************************************************************

// Validate raw data array for consistency and reasonable values
bool validate_raw_data(const raw_data_t *data, int count) {
  if (!data || count <= 0) {
    return false;
  }

  // Check for reasonable sample indices (should be sequential or increasing)
  for (int i = 1; i < count; i++) {
    if (data[i].sample_index < data[i-1].sample_index) {
      // Non-monotonic indices - could be valid but unusual
      return true; // Still valid, just not sequential
    }
  }

  return true;
}

// Validate squiggle data array for consistency and reasonable values
bool validate_squiggle_data(const squiggle_data_t *data, int count) {
  if (!data || count <= 0) {
    return false;
  }

  // Check for reasonable values
  for (int i = 0; i < count; i++) {
    // Position should be reasonable
    if (data[i].pos < 0) {
      return false;
    }

    // Base should be valid DNA/RNA base
    char base = data[i].base;
    if (base != 'A' && base != 'T' && base != 'G' && base != 'C' &&
        base != 'U' && base != 'N') {
      return false;
    }

    // Dwell time should be positive
    if (data[i].dwell <= 0.0f) {
      return false;
    }

    // Standard deviation should be non-negative
    if (data[i].sd < 0.0f) {
      return false;
    }
  }

  return true;
}

// **********************************************************************
// Data Conversion Utilities
// **********************************************************************

// Convert raw ADC samples to picoampere current using calibration parameters
int convert_raw_to_current(raw_data_t *data, int count, float offset, float range, float digitisation) {
  if (!data || count <= 0 || digitisation == 0.0f) {
    return -1;
  }

  // Apply conversion: signal_pA = (raw_signal + offset) * range / digitisation
  for (int i = 0; i < count; i++) {
    data[i].raw_value = (data[i].raw_value + offset) * range / digitisation;
  }

  return 0;
}