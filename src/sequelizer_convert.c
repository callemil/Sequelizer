// **********************************************************************
// sequelizer_convert.c
// **********************************************************************
// S Magierowski Aug 22 2025
//
// File format conversion operations for Sequelizer
// cmake ..
// cmake --build .
// ./sequelizer convert --help
// ./sequelizer convert input.fast5 --to metadata -o output.txt
// ./sequelizer convert /path/to/fast5_files/ --to raw --recursive -o output.txt
// ./sequelizer convert /Users/seb/Research/Scraps/scrappie_2016_11_28_73e3/reads/read_ch228_file118.fast5 --to raw -o signal.txt
// ./sequelizer convert /Users/seb/Documents/GitHub/SquiggleFilter/data/lambda/fast5/FAL11227_e2243762ddcab66a4299cc8b21f76b3f66c41f01_0.fast5 --to raw -o signals/

#include "sequelizer_convert.h"
#include "core/fast5_io.h"
#include "core/fast5_utils.h"
#include "core/fast5_convert.h"
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

static int convert_to_raw(char **files, size_t file_count, const char *output_file, bool verbose, bool all_reads) {
  return extract_raw_signals(files, file_count, output_file, all_reads, verbose);
}

// **********************************************************************
// Argument Parsing
// **********************************************************************

static char doc[] = "sequelizer convert -- Extract raw signals from Fast5 files\v"
"EXAMPLES:\n"
"  sequelizer convert single.fast5 --to raw -o signal.txt\n"
"  sequelizer convert multi.fast5 --to raw -o signals/\n"
"  sequelizer convert multi.fast5 --to raw -o signals/ --all";

static char args_doc[] = "INPUT";

static struct argp_option options[] = {
  {"to",            't', "FORMAT",  0, "Output format: raw (default)"},
  {"output",        'o', "FILE",    0, "Output file or directory"},
  {"all",           'a', 0,         0, "Extract all reads (default: first 3 for multi-read)"},
  {"recursive",     'r', 0,         0, "Search directories recursively"},
  {"verbose",       'v', 0,         0, "Show detailed information"},
  {0}
};

struct arguments {
  char *input_path;
  char *output_format;
  char *output_file;
  bool all;
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
    case 'a':
      arguments->all = true;
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

static struct argp convert_argp = {options, parse_opt, args_doc, doc};

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
  arguments.output_format = "raw";
  arguments.output_file = NULL;
  arguments.all = false;
  arguments.recursive = false;
  arguments.verbose = false;
  
  // Parse command line arguments using argp framework
  argp_parse(&convert_argp, argc, argv, 0, 0, &arguments);
  
  // ========================================================================
  // STEP 2: VALIDATE ARGUMENTS
  // ========================================================================
  
  // Validate output format
  if (strcmp(arguments.output_format, "raw") != 0) {
    errx(EXIT_FAILURE, "Invalid output format '%s'. Supported formats: raw", 
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
  
  int result = convert_to_raw(input_files, file_count, arguments.output_file, arguments.verbose, arguments.all);
  
  // ========================================================================
  // STEP 5: CLEANUP ALL ALLOCATED RESOURCES
  // ========================================================================
  free_file_list(input_files, file_count);
  
  return result;
}