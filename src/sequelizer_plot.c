// **********************************************************************
// sequelizer_plot.c
// **********************************************************************
// S Magierowski Sep 27 2025
//
// Signal visualization and plotting operations for Sequelizer
// cmake ..
// cmake --build .
// ./sequelizer plot --help
// ./sequelizer plot input.txt
// ./sequelizer plot input.txt --output plot.png
// ./sequelizer plot --raw input.txt

#include "sequelizer_plot.h"
#include "core/fast5_io.h"
#include "core/fast5_utils.h"
#include "core/util.h"
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>
#include <argp.h>
#include <err.h>

// **********************************************************************
// Data Structures and File Format Detection
// **********************************************************************

typedef enum {
  FILE_FORMAT_UNKNOWN,
  FILE_FORMAT_RAW,
  FILE_FORMAT_SQUIGGLE
} file_format_t;

typedef struct {
  int sample_index;
  float raw_value;
} raw_data_t;

// Detect file format by examining first few lines
static file_format_t detect_file_format(FILE *fp) {
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

// Parse raw signal file (like output from sequelizer convert)
static int parse_raw_file(FILE *fp, raw_data_t **out_data) {
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

// Plot raw signal data using feedgnuplot
static int plot_raw_data(raw_data_t *data, int count, const char *title) {
  char cmd[512];
  const char *plot_title = title ? title : "Raw Signal Data";

  // Calculate x-axis range from data
  int min_index = data[0].sample_index;
  int max_index = data[0].sample_index;
  for (int i = 1; i < count; i++) {
    if (data[i].sample_index < min_index) min_index = data[i].sample_index;
    if (data[i].sample_index > max_index) max_index = data[i].sample_index;
  }

  // Use feedgnuplot for interactive plotting
  snprintf(cmd, sizeof(cmd),
           "feedgnuplot --lines --domain --title \"%s\" --xlabel \"Sample Index\" --ylabel \"Raw Value\" --xmin %d --xmax %d",
           plot_title, min_index, max_index);

  FILE *pipe = popen(cmd, "w");
  if (!pipe) {
    fprintf(stderr, "Error opening pipe to feedgnuplot\n");
    fprintf(stderr, "Make sure feedgnuplot is installed: brew install feedgnuplot\n");
    return -1;
  }

  // Send data to feedgnuplot
  fprintf(pipe, "# sample_index raw_value\n");
  for (int i = 0; i < count; i++) {
    fprintf(pipe, "%d %f\n", data[i].sample_index, data[i].raw_value);
  }

  int result = pclose(pipe);
  if (result != 0) {
    fprintf(stderr, "Warning: plotting process returned code %d\n", result);
  }
  return 0;
}

// **********************************************************************
// Main Plotting Function
// **********************************************************************

static int plot_signals(char **files, int file_count, const char *output_file, bool verbose) {
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

    // Detect file format
    file_format_t format = detect_file_format(fh);
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
          plot_raw_data(raw_data, data_count, files[fn]);
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

  return EXIT_SUCCESS;
}

// **********************************************************************
// Argument Parsing
// **********************************************************************

static char doc[] = "sequelizer plot -- Signal visualization and plotting\v"
"EXAMPLES:\n"
"  sequelizer plot single.txt\n"
"  sequelizer plot multi.txt --output plot.png\n"
"  sequelizer plot --raw signals.txt";

static char args_doc[] = "data_file [data_file ...]";

static struct argp_option options[] = {
  {"output",        'o', "FILE",    0, "Output file for plot"},
  {"raw",           'r', 0,         0, "Plot raw signals"},
  {"verbose",       'v', 0,         0, "Show detailed information"},
  {0}
};

struct arguments {
  char *output_file;
  bool raw;
  bool verbose;
  char **files;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;

  switch (key) {
    case 'o':
      arguments->output_file = arg;
      break;
    case 'r':
      arguments->raw = true;
      break;
    case 'v':
      arguments->verbose = true;
      break;
    case ARGP_KEY_NO_ARGS:
      argp_usage(state);
      break;
    case ARGP_KEY_ARG:
      arguments->files = &state->argv[state->next - 1];
      state->next = state->argc;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp plot_argp = {options, parse_opt, args_doc, doc};

// **********************************************************************
// Main Function
// **********************************************************************

int main_plot(int argc, char *argv[]) {

  // ========================================================================
  // STEP 1: PARSE COMMAND LINE ARGUMENTS AND SET DEFAULTS
  // ========================================================================
  struct arguments arguments;

  // Set sensible defaults for all configuration options
  arguments.output_file = NULL;
  arguments.raw = false;
  arguments.verbose = false;
  arguments.files = NULL;

  // Parse command line arguments using argp framework
  argp_parse(&plot_argp, argc, argv, 0, 0, &arguments);

  // ========================================================================
  // STEP 2: DETERMINE NUMBER OF INPUT FILES TO PROCESS
  // ========================================================================
  // Note: Unlike other sequelizer commands, plot works on text files (not Fast5)
  // following Ciren's pattern of accepting direct file arguments
  int file_count = 0;
  for (; arguments.files[file_count]; file_count++) ;

  if (arguments.verbose) {
    printf("Found %d files to plot\n", file_count);
    if (arguments.output_file) {
      printf("Output file: %s\n", arguments.output_file);
    }
    printf("Plot type: %s\n", arguments.raw ? "raw signals" : "processed signals");
    printf("\n");
  }

  // ========================================================================
  // STEP 3: PERFORM PLOTTING
  // ========================================================================

  int result = plot_signals(arguments.files, file_count, arguments.output_file, arguments.verbose);

  // ========================================================================
  // STEP 4: NO CLEANUP NEEDED (FILES ARE JUST POINTERS TO ARGV)
  // ========================================================================
  // Note: Unlike Fast5 file discovery, no dynamic allocation to free

  return result;
}