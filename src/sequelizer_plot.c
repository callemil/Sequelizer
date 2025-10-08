// **********************************************************************
// sequelizer_plot.c
// **********************************************************************
// Sebastian Claudiusz Magierowski Sep 27 2025
//
/* Signal visualization and plotting operations for Sequelizer
cmake .. & cmake --build .
./sequelizer plot --help
./sequelizer plot file.txt
./sequelizer plot signal.txt --verbose
./sequeilzer plot file1.txt file2.txt
./sequelizer plot ./results/*
TODO
./sequelizer plot input.txt --output plot.png
./sequelizer plot --raw input.txt
NOTES
file format detection happens in plot_utils.c/h
*/
#include "sequelizer_plot.h"
#include "core/fast5_io.h"
#include "core/fast5_utils.h"
#include "core/util.h"
#include "core/plot_utils.h"
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>
#include <argp.h>
#include <err.h>
#include <assert.h>

// **********************************************************************
// Argument Parsing
// **********************************************************************
static char doc[] = "sequelizer plot -- Signal visualization and plotting\v"
"EXAMPLES:\n"
"  sequelizer plot single.txt\n"
"  sequelizer plot data.txt --output plot.png\n"
"  sequelizer plot --png signals.txt\n"
"  sequelizer plot --title \"My Data\" file.txt\n"
"  sequelizer plot --limit 5 --verbose file1.txt file2.txt";

static char args_doc[] = "data_file [data_file ...]";

static struct argp_option options[] = {
  {"limit",         'l', "NREADS",  0, "Maximum number of reads to process (0 is unlimited)"},
  {"output",        'o', "FILE",    0, "Output file for plot"},
  {"png",           'p', 0,         0, "Generate PNG files instead of interactive plots"},
  {"title",         't', "STRING",  0, "Plot title"},
  {"text-only",      1,  0,         0, "Output parsed data as text only (no plots)"},
  {"verbose",       'v', 0,         0, "Show detailed information"},
  {0}
};

struct arguments {
  int limit;
  char *output_file;
  char *title;
  bool png_mode;
  bool text_only;
  bool verbose;
  char **files;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;

  switch (key) {
    case 'l':
      arguments->limit = atoi(arg);
      assert(arguments->limit > 0);
      break;
    case 'o':
      arguments->output_file = arg;
      break;
    case 'p':
      arguments->png_mode = true;
      break;
    case 't':
      arguments->title = arg;
      break;
    case 1:
      arguments->text_only = true;
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
// Plot Function
// **********************************************************************
// Takes parsed data arary, constructs feedgnuplot command, pipes data to feedgnuplot for rendering
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
// Main Function
// **********************************************************************
int main_plot(int argc, char *argv[]) {

  // ========================================================================
  // STEP 1: PARSE COMMAND LINE ARGUMENTS AND SET DEFAULTS
  // ========================================================================
  struct arguments arguments;

  // Set sensible defaults for all configuration options
  arguments.limit = 0;
  arguments.output_file = NULL;
  arguments.title = NULL;
  arguments.png_mode = false;
  arguments.text_only = false;
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
    if (arguments.title) {
      printf("Plot title: %s\n", arguments.title);
    }
    printf("Mode: %s\n", arguments.png_mode ? "PNG generation" : "interactive plotting");
    if (arguments.text_only) {
      printf("Text-only mode enabled\n");
    }
    if (arguments.limit > 0) {
      printf("Read limit: %d\n", arguments.limit);
    }
    printf("\n");
  }

  // ========================================================================
  // STEP 3: PERFORM PLOTTING
  // ========================================================================

  // Apply limit if specified
  int actual_file_count = file_count;
  if (arguments.limit > 0 && arguments.limit < file_count) {
    actual_file_count = arguments.limit;
    if (arguments.verbose) {
      printf("Limiting processing to %d files (out of %d available)\n\n", actual_file_count, file_count);
    }
  }

  int result = plot_signals(arguments.files, actual_file_count, arguments.output_file, arguments.verbose, plot_raw_data);

  // ========================================================================
  // STEP 4: NO CLEANUP NEEDED (FILES ARE JUST POINTERS TO ARGV)
  // ========================================================================
  // Note: Unlike Fast5 file discovery, no dynamic allocation to free

  return result;
}