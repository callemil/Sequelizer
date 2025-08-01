// **********************************************************************
// sequelizer_subcommands.c
// **********************************************************************
// S Magierowski Jul 26 2025
//
#include "../include/sequelizer.h"
#include "sequelizer_subcommands.h"

enum sequelizer_mode get_sequelizer_mode(const char *modestr) {
  if (0 == strcmp(modestr, "help")) {
    return SEQUELIZER_MODE_HELP;
  }
  if (0 == strcmp(modestr, "seqgen")) {
    return SEQUELIZER_MODE_SEQGEN;
  }
  if (0 == strcmp(modestr, "fast5")) {
    return SEQUELIZER_MODE_FAST5;
  }
  return SEQUELIZER_MODE_INVALID;
}

const char *sequelizer_mode_string(const enum sequelizer_mode mode) {
  switch (mode) {
    case SEQUELIZER_MODE_HELP:
      return "help";
    case SEQUELIZER_MODE_SEQGEN:
      return "seqgen";
    case SEQUELIZER_MODE_FAST5:
      return "fast5";
    case SEQUELIZER_MODE_INVALID:
      errx(EXIT_FAILURE, "Invalid Sequelizer mode\n");
    default:
      errx(EXIT_FAILURE, "Sequelizer enum failure -- report bug\n");
  }
  return NULL;
}

const char *sequelizer_mode_description(const enum sequelizer_mode mode) {
  switch (mode) {
    case SEQUELIZER_MODE_HELP:
      return "Show help message";
    case SEQUELIZER_MODE_SEQGEN:
      return "Generate synthetic sequences and signals";
    case SEQUELIZER_MODE_FAST5:
      return "Fast5 file operations (coming soon)";
    case SEQUELIZER_MODE_INVALID:
      errx(EXIT_FAILURE, "Invalid Sequelizer mode\n");
    default:
      errx(EXIT_FAILURE, "Sequelizer enum failure -- report bug\n");
  }
  return NULL;
}

int fprint_sequelizer_commands(FILE * fp, bool header) {
  if (header) {
    fprintf(fp, "COMMANDS:\n");
  }
  for (enum sequelizer_mode mode = SEQUELIZER_MODE_HELP; mode < sequelizer_ncommand; mode++) {
    fprintf(fp, "  %-8s %s\n", sequelizer_mode_string(mode), sequelizer_mode_description(mode));
  }
  return EXIT_SUCCESS;
}

int main_help_short(void) {
  printf("Sequelizer - DNA Sequence Analysis Toolkit\n\n");
  printf("Basic usage:\n");
  printf("* sequelizer help          Print detailed help\n");
  printf("* sequelizer seqgen        Generate synthetic sequences and signals\n");
  printf("* sequelizer fast5         Fast5 file operations\n");
  printf("\nFor more information: sequelizer help\n");
  return EXIT_SUCCESS;
}

int main_seqgen(int argc, char *argv[]) {
  // Handle help for seqgen subcommand
  if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
    printf("Sequelizer seqgen - Sequence Generation\n");
    printf("=======================================\n\n");
    printf("Generate synthetic sequences and signals for testing and development.\n\n");
    printf("Usage: sequelizer seqgen [options]\n\n");
    printf("Options:\n");
    printf("  --nanopore        Generate nanopore-style signals\n");
    printf("  --model MODEL     Use specific model (r94, r10, etc.)\n");
    printf("  --length LENGTH   Sequence length to generate\n");
    printf("  -o FILE           Output file\n");
    printf("  --help, -h        Show this help\n\n");
    printf("Examples:\n");
    printf("  sequelizer seqgen --nanopore --length 1000 -o signals.fast5\n");
    printf("  sequelizer seqgen --model r94 --length 500\n");
    return EXIT_SUCCESS;
  }

  printf("Sequelizer seqgen - Sequence Generation\n");
  printf("=======================================\n\n");
  printf("This will generate synthetic sequences and signals.\n");
  printf("Implementation coming soon...\n\n");

  // For now, just show what the interface will look like
  printf("Planned usage:\n");
  printf("  sequelizer seqgen --nanopore --model r94 --length 1000 -o signals.fast5\n");
  printf("  sequelizer seqgen --ecg --duration 30s --heart-rate 75 -o cardiac.csv\n");
  printf("  sequelizer seqgen --audio --type speech --duration 10s -o voice.wav\n\n");
  printf("Use 'sequelizer seqgen --help' for detailed options.\n");

  return EXIT_SUCCESS;
}

int main_fast5(int argc, char *argv[]) {
  // Handle help for fast5 subcommand
  if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
    printf("Sequelizer fast5 - Fast5 File Operations\n");
    printf("========================================\n\n");
    printf("Tools for working with Fast5 nanopore data files.\n\n");
    printf("Usage: sequelizer fast5 [command] [options]\n\n");
    printf("Commands:\n");
    printf("  info      Display Fast5 file information\n");
    printf("  convert   Convert between Fast5 formats\n");
    printf("  extract   Extract signals or metadata\n");
    printf("  validate  Validate Fast5 file integrity\n\n");
    printf("Options:\n");
    printf("  --help, -h        Show this help\n\n");
    printf("Examples:\n");
    printf("  sequelizer fast5 info data.fast5\n");
    printf("  sequelizer fast5 convert --to-multi input.fast5 -o output.fast5\n");
    printf("  sequelizer fast5 extract --signals data.fast5 -o signals.csv\n");
    return EXIT_SUCCESS;
  }

  printf("Sequelizer fast5 - Fast5 File Operations\n");
  printf("========================================\n\n");
  printf("Fast5 file processing tools for nanopore data.\n");
  printf("Implementation coming soon...\n\n");

  printf("Planned functionality:\n");
  printf("• File format validation and conversion\n");
  printf("• Signal extraction and analysis\n");
  printf("• Metadata inspection and reporting\n");
  printf("• Batch processing capabilities\n\n");
  printf("Use 'sequelizer fast5 --help' for detailed options.\n");

  return EXIT_SUCCESS;
}