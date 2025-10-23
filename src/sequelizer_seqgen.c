// **********************************************************************
// sequelizer_seqgen.c
// **********************************************************************
// Sebastian Claudiusz Magierowski Oct 15 2025
/*
Sequence generation and signal simulation for Sequelizer. A small change.
Example uses:
./sequelizer seqgen --help
./sequelizer seqgen input.fasta --output signals.fast5
./sequelizer seqgen --generate --seq-length 1000 --num-sequences 10 -o test.fast5
*/
#include "sequelizer_seqgen.h"
#include <stdio.h>
#include <stdbool.h>
#include <argp.h>

// **********************************************************************
// Argument Parsing
// **********************************************************************
static char doc[] = "sequelizer seqgen -- Signal generation from DNA sequences\v"
"EXAMPLES:\n"
"  sequelizer seqgen input.fasta -o signals.fast5\n"
"  sequelizer seqgen --generate --seq-length 1000 -o test.fast5\n"
"NOTE: This command is not yet implemented.";

static char args_doc[] = "[INPUT]";

static struct argp_option options[] = {
  {"output",        'o', "FILE",    0, "Output file"},
  {"verbose",       'v', 0,         0, "Show detailed information"},
  {0}
};

struct arguments {
  char *input_file;
  char *output_file;
  bool verbose;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;

  switch (key) {
    case 'o':
      arguments->output_file = arg;
      break;
    case 'v':
      arguments->verbose = true;
      break;
    case ARGP_KEY_ARG:
      if (state->arg_num >= 1) {
        argp_usage(state);
      }
      arguments->input_file = arg;
      break;
    case ARGP_KEY_END:
      // Input file is optional for now
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
int main_seqgen(int argc, char *argv[]) {

  // ========================================================================
  // STEP 1: PARSE COMMAND LINE ARGUMENTS AND SET DEFAULTS
  // ========================================================================
  struct arguments arguments;

  // Set sensible defaults for all configuration options
  arguments.input_file = NULL;
  arguments.output_file = NULL;
  arguments.verbose = false;

  // Parse command line arguments using argp framework
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  // ========================================================================
  // STEP 2: DISPLAY NOT IMPLEMENTED MESSAGE
  // ========================================================================
  printf("\n");
  printf("====================================================\n");
  printf("  SEQUELIZER SEQGEN - NOT YET IMPLEMENTED\n");
  printf("====================================================\n");
  printf("\n");
  printf("This command is currently under development.\n");
  printf("\n");
  printf("The seqgen command will provide signal generation\n");
  printf("capabilities from DNA sequences, including:\n");
  printf("\n");
  printf("  - Fast5 file generation from FASTA input\n");
  printf("  - Random sequence generation for testing\n");
  printf("  - Signal simulation with realistic noise\n");
  printf("  - Support for custom squiggle models\n");
  printf("\n");
  printf("For now, please use the ciren seqgen command for\n");
  printf("signal generation functionality.\n");
  printf("\n");
  printf("====================================================\n");
  printf("\n");

  return 0;
}
