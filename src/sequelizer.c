// **********************************************************************
// Sequelizer: Open Source DNA Sequence Analysis Toolkit
// **********************************************************************
// S Magierowski Jul 26 2025
//
// to build:
// mkdir build
// cd build
// cmake ..
// cmake --build .
// Get general command list: sequelizer help
// Or run your command & subcommand
// e.g.,
// sequelizer seqgen --nanopore --length 1000 -o synthetic.fast5

#include <string.h>   /*  ←  for strcmp  */
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include "../include/sequelizer.h"

int main(int argc, char *argv[]) {

  /* 1. No arguments → short help  */
  if (argc == 1) {
    return main_help_short();
  }

  /* 2. Handle `--help` or `-h` **before** sub-command switch  */
  if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
    return main_help_short();

  /* 3. Normal sub-command dispatch */
  int ret = EXIT_FAILURE;
  switch (get_sequelizer_mode(argv[1])) {
    case SEQUELIZER_MODE_SEQGEN:
      ret = main_seqgen(argc - 1, argv + 1);
      break;
    case SEQUELIZER_MODE_HELP:
      ret = main_help_long();
      break;
    default:
      ret = EXIT_FAILURE;
      warnx("Unrecognised subcommand '%s'\n", argv[1]);
      printf("Try 'sequelizer help' for available commands.\n");
  }
  return ret;
}

// Help functions
int main_help_short(void) {
  printf("Sequelizer - Open Source DNA Sequence Analysis Toolkit\n\n");
  printf("Basic usage:\n");
  printf("* sequelizer help          Print detailed help\n");
  printf("* sequelizer seqgen        Generate synthetic sequences and signals\n");
  printf("\nFor more information: sequelizer help\n");
  return EXIT_SUCCESS;
}

int main_help_long(void) {
  printf("Sequelizer - Open Source DNA Sequence Analysis Toolkit\n");
  printf("========================================================\n\n");
  printf("COMMANDS:\n");
  printf("  seqgen    Generate synthetic sequences and signals\n");
  printf("            Examples:\n");
  printf("              sequelizer seqgen --nanopore --length 1000\n");
  printf("              sequelizer seqgen --help (for more options)\n\n");
  printf("  help      Show this help message\n\n");
  printf("GENERAL OPTIONS:\n");
  printf("  --help, -h    Show help for any command\n");
  printf("  --version     Show version information\n\n");
  printf("For more information, visit: https://github.com/callemil/Sequelizer\n");
  return EXIT_SUCCESS;
}

// Mode detection
sequelizer_mode_t get_sequelizer_mode(const char *mode_string) {
  if (strcmp(mode_string, "seqgen") == 0) {
    return SEQUELIZER_MODE_SEQGEN;
  }
  if (strcmp(mode_string, "help") == 0) {
    return SEQUELIZER_MODE_HELP;
  }
  return SEQUELIZER_MODE_UNKNOWN;
}

// Placeholder for seqgen - we'll implement this next
int main_seqgen(int argc, char *argv[]) {
  printf("Sequelizer seqgen - Sequence Generation\n");
  printf("=======================================\n\n");
  printf("This will generate synthetic sequences and signals.\n");
  printf("Implementation coming soon...\n\n");

  // For now, just show what the interface will look like
  printf("Planned usage:\n");
  printf("  sequelizer seqgen --nanopore --model r94 --length 1000 -o signals.fast5\n");
  printf("  sequelizer seqgen --ecg --duration 30s --heart-rate 75 -o cardiac.csv\n");
  printf("  sequelizer seqgen --audio --type speech --duration 10s -o voice.wav\n");

  return EXIT_SUCCESS;
}
