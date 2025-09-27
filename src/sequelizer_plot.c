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

static int plot_signals(char **files, int file_count, const char *output_file, bool verbose) {
  printf("Plotting functionality will be implemented here...\n");
  printf("Files to process: %d\n", file_count);
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