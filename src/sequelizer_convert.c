// **********************************************************************
// sequelizer_convert.c
// **********************************************************************
// S Magierowski Aug 22 2025
//
// File format conversion operations for Sequelizer
// 
// ./sequelizer convert input.fast5 --to metadata -o output.txt
// ./sequelizer convert /path/to/fast5_files/ --to raw --recursive -o output.txt

#include "sequelizer_convert.h"
#include "core/fast5_io.h"
#include "core/fast5_utils.h"
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>
#include <sys/time.h>
#include <argp.h>
#include <err.h>

// **********************************************************************
// Progress Bar Functions
// **********************************************************************

static void display_progress(int completed, int total, bool verbose) {
  if (total == 0) return;
  
  int percent = (completed * 100) / total;
  int bar_width = 40;
  int filled = (completed * bar_width) / total;
  
  printf("\r[");
  for (int i = 0; i < bar_width; i++) {
    printf(i < filled ? "█" : "░");
  }
  printf("] %d%% (%d/%d)", percent, completed, total);
  
  if (verbose) {
    printf(" converting files");
  }
  
  fflush(stdout);
}

// **********************************************************************
// Format Conversion Functions
// **********************************************************************

static int convert_to_metadata(char **files, size_t file_count, const char *output_file, bool verbose) {
  printf("Converting %zu files to metadata format...\n", file_count);
  
  // TODO: Implement metadata format conversion
  // This should extract and format metadata from Fast5 files
  
  if (verbose) {
    printf("Metadata conversion not yet implemented.\n");
  }
  
  return EXIT_SUCCESS;
}

static int convert_to_raw(char **files, size_t file_count, const char *output_file, bool verbose) {
  printf("Converting %zu files to raw format...\n", file_count);
  
  // TODO: Implement raw format conversion
  // This should extract raw signal data from Fast5 files
  
  if (verbose) {
    printf("Raw format conversion not yet implemented.\n");
  }
  
  return EXIT_SUCCESS;
}

// **********************************************************************
// Argument Parsing
// **********************************************************************

const char *argp_program_version = "sequelizer convert 1.0";
const char *argp_program_bug_address = "magierowski@gmail.com";

static char doc[] = "sequelizer convert -- File format conversion\v"
"EXAMPLES:\n"
"  sequelizer convert data.fast5 --to metadata -o output.txt\n"
"  sequelizer convert /path/to/fast5_files/ --to raw --recursive -o signals.txt\n"
"  sequelizer convert input.fast5 --to metadata --verbose";

static char args_doc[] = "INPUT";

static struct argp_option options[] = {
  {"to",            't', "FORMAT",  0, "Output format: metadata, raw"},
  {"output",        'o', "FILE",    0, "Output file"},
  {"recursive",     'r', 0,         0, "Search directories recursively"},
  {"verbose",       'v', 0,         0, "Show detailed information"},
  {0}
};

struct arguments {
  char *input_path;
  char *output_format;
  char *output_file;
  bool recursive;
  bool verbose;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;
  
  switch (key) {
    case 't':
      arguments->output_format = arg;
      break;
    case 'o':
      arguments->output_file = arg;
      break;
    case 'r':
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

static struct argp argp = {options, parse_opt, args_doc, doc};

// **********************************************************************
// Main Function 
// **********************************************************************

int main_convert(int argc, char *argv[]) {

  // ========================================================================
  // STEP 1: PARSE COMMAND LINE ARGUMENTS AND SET DEFAULTS
  // ========================================================================
  struct arguments arguments;
  
  // Set sensible defaults for all configuration options
  arguments.input_path = NULL;
  arguments.output_format = "metadata";
  arguments.output_file = NULL;
  arguments.recursive = false;
  arguments.verbose = false;
  
  // Parse command line arguments using argp framework
  argp_parse(&argp, argc, argv, 0, 0, &arguments);
  
  // ========================================================================
  // STEP 2: VALIDATE ARGUMENTS
  // ========================================================================
  
  // Validate output format
  if (strcmp(arguments.output_format, "metadata") != 0 && 
      strcmp(arguments.output_format, "raw") != 0) {
    errx(EXIT_FAILURE, "Invalid output format '%s'. Supported formats: metadata, raw", 
         arguments.output_format);
  }
  
  // ========================================================================
  // STEP 3: DISCOVER AND ENUMERATE INPUT FILES
  // ========================================================================
  size_t file_count = 0;
  char **input_files = find_fast5_files(arguments.input_path, arguments.recursive, &file_count);
  
  // Handle case where no files are found
  if (!input_files || file_count == 0) {
    printf("No Fast5 files found.\n");
    return EXIT_SUCCESS;
  }
  
  if (arguments.verbose) {
    printf("Found %zu files to convert\n", file_count);
    printf("Output format: %s\n", arguments.output_format);
    if (arguments.output_file) {
      printf("Output file: %s\n", arguments.output_file);
    }
    printf("\n");
  }
  
  // ========================================================================
  // STEP 4: PERFORM FORMAT CONVERSION
  // ========================================================================
  
  int result = EXIT_SUCCESS;
  
  if (strcmp(arguments.output_format, "metadata") == 0) {
    result = convert_to_metadata(input_files, file_count, arguments.output_file, arguments.verbose);
  } else if (strcmp(arguments.output_format, "raw") == 0) {
    result = convert_to_raw(input_files, file_count, arguments.output_file, arguments.verbose);
  }
  
  // ========================================================================
  // STEP 5: CLEANUP ALL ALLOCATED RESOURCES
  // ========================================================================
  free_file_list(input_files, file_count);
  
  return result;
}