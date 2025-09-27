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
// Plotting Functions
// **********************************************************************

static int plot_signals(char **files, size_t file_count, const char *output_file, bool verbose) {
  printf("Plotting functionality will be implemented here...\n");
  printf("Files to process: %zu\n", file_count);
  if (output_file) {
    printf("Output file: %s\n", output_file);
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

static char args_doc[] = "INPUT";

static struct argp_option options[] = {
  {"output",        'o', "FILE",    0, "Output file for plot"},
  {"raw",           'r', 0,         0, "Plot raw signals"},
  {"recursive",     'R', 0,         0, "Search directories recursively"},
  {"verbose",       'v', 0,         0, "Show detailed information"},
  {0}
};

struct arguments {
  char *input_path;
  char *output_file;
  bool raw;
  bool recursive;
  bool verbose;
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
    case 'R':
      arguments->recursive = true;
      break;
    case 'v':
      arguments->verbose = true;
      break;
    case ARGP_KEY_ARG:
      if (state->arg_num >= 1) {
        argp_usage(state);
      }
      arguments->input_path = arg;
      break;
    case ARGP_KEY_END:
      if (state->arg_num < 1) {
        argp_usage(state);
      }
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
  arguments.input_path = NULL;
  arguments.output_file = NULL;
  arguments.raw = false;
  arguments.recursive = false;
  arguments.verbose = false;

  // Parse command line arguments using argp framework
  argp_parse(&plot_argp, argc, argv, 0, 0, &arguments);

  // ========================================================================
  // STEP 2: DISCOVER AND ENUMERATE INPUT FILES
  // ========================================================================
  size_t file_count = 0;
  char **input_files = find_fast5_files(arguments.input_path, arguments.recursive, &file_count);

  // Handle case where no files are found
  if (!input_files || file_count == 0) {
    printf("No Fast5 files found.\n");
    return EXIT_SUCCESS;
  }

  if (arguments.verbose) {
    printf("Found %zu files to plot\n", file_count);
    if (arguments.output_file) {
      printf("Output file: %s\n", arguments.output_file);
    }
    printf("Plot type: %s\n", arguments.raw ? "raw signals" : "processed signals");
    printf("\n");
  }

  // ========================================================================
  // STEP 3: PERFORM PLOTTING
  // ========================================================================

  int result = plot_signals(input_files, file_count, arguments.output_file, arguments.verbose);

  // ========================================================================
  // STEP 4: CLEANUP ALL ALLOCATED RESOURCES
  // ========================================================================
  free_file_list(input_files, file_count);

  return result;
}