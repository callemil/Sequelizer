// **********************************************************************
// sequelizer_subcommands.c
// **********************************************************************
// S Magierowski Jul 26 2025
//
#include "../include/sequelizer.h"
#include "sequelizer_subcommands.h"
#include "core/fast5_io.h"
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>

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
      return "Fast5 file operations";
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

// Helper function to display file information
static void display_fast5_info(const char *filename, bool verbose) {
  printf("Fast5 File: %s\n", filename);
  printf("=====================================\n");
  
  // Get metadata for all reads in the file
  size_t metadata_count = 0;
  fast5_metadata_t *metadata = read_fast5_metadata(filename, &metadata_count);
  
  if (!metadata || metadata_count == 0) {
    printf("Error: Could not read metadata from file\n\n");
    return;
  }
  
  // File format and basic info
  printf("Format: %s\n", metadata[0].is_multi_read ? "Multi-read" : "Single-read");
  printf("Reads: %zu\n", metadata_count);
  
  if (metadata_count > 0) {
    printf("Sample rate: %.0f Hz\n", metadata[0].sample_rate);
  }
  
  // Calculate statistics
  uint32_t total_signal_length = 0;
  uint32_t min_signal_length = UINT32_MAX;
  uint32_t max_signal_length = 0;
  double total_duration = 0.0;
  
  for (size_t i = 0; i < metadata_count; i++) {
    total_signal_length += metadata[i].signal_length;
    if (metadata[i].signal_length < min_signal_length) {
      min_signal_length = metadata[i].signal_length;
    }
    if (metadata[i].signal_length > max_signal_length) {
      max_signal_length = metadata[i].signal_length;
    }
    total_duration += metadata[i].duration;
  }
  
  double avg_signal_length = (double)total_signal_length / metadata_count;
  
  printf("Signal statistics:\n");
  printf("  Total samples: %u\n", total_signal_length);
  printf("  Average length: %.0f samples\n", avg_signal_length);
  printf("  Range: %u - %u samples\n", min_signal_length, max_signal_length);
  
  if (verbose) {
    double total_time_seconds = 0.0;
    for (size_t i = 0; i < metadata_count; i++) {
      if (metadata[i].sample_rate > 0) {
        total_time_seconds += metadata[i].signal_length / metadata[i].sample_rate;
      }
    }
    double avg_time_seconds = total_time_seconds / metadata_count;
    
    printf("  Total duration: %.1f seconds\n", total_time_seconds);
    printf("  Average duration: %.1f seconds\n", avg_time_seconds);
    
    // Show file size information
    struct stat file_stat;
    if (stat(filename, &file_stat) == 0) {
      double file_size_mb = (double)file_stat.st_size / (1024.0 * 1024.0);
      printf("  File size: %.2f MB\n", file_size_mb);
      if (total_signal_length > 0) {
        double compression_ratio = (double)file_stat.st_size / (total_signal_length * sizeof(float));
        printf("  Compression ratio: %.2fx\n", compression_ratio);
      }
    }
  } else {
    double total_time_seconds = 0.0;
    for (size_t i = 0; i < metadata_count; i++) {
      if (metadata[i].sample_rate > 0) {
        total_time_seconds += metadata[i].signal_length / metadata[i].sample_rate;
      }
    }
    printf("  Total duration: %.1f seconds\n", total_time_seconds);
  }
  
  // Show individual read details based on verbose mode and read count
  if (verbose) {
    printf("\nDetailed read information:\n");
    for (size_t i = 0; i < metadata_count; i++) {
      printf("  Read %zu: %s\n", i + 1, 
             metadata[i].read_id ? metadata[i].read_id : "unknown");
      printf("    Signal length: %u samples\n", metadata[i].signal_length);
      printf("    Duration: %u samples\n", metadata[i].duration);
      printf("    Read number: %u\n", metadata[i].read_number);
      if (metadata[i].sample_rate > 0) {
        double read_time_seconds = metadata[i].signal_length / metadata[i].sample_rate;
        printf("    Time: %.2f seconds\n", read_time_seconds);
      }
    }
  } else if (metadata_count <= 3) {
    printf("\nRead details:\n");
    for (size_t i = 0; i < metadata_count; i++) {
      printf("  Read %zu: %s (%u samples)\n", i + 1,
             metadata[i].read_id ? metadata[i].read_id : "unknown",
             metadata[i].signal_length);
    }
  } else {
    printf("\nShowing first 3 reads (use --verbose for all):\n");
    for (size_t i = 0; i < 3; i++) {
      printf("  Read %zu: %s (%u samples)\n", i + 1,
             metadata[i].read_id ? metadata[i].read_id : "unknown",
             metadata[i].signal_length);
    }
    printf("  ... and %zu more reads\n", metadata_count - 3);
  }
  
  printf("\n");
  free_fast5_metadata(metadata, metadata_count);
}

// Helper function to process directory
static void process_directory(const char *directory, bool recursive, bool verbose) {
  printf("Fast5 Directory Analysis\n");
  printf("========================\n");
  printf("Directory: %s\n", directory);
  printf("Recursive: %s\n\n", recursive ? "yes" : "no");
  
  // Find all Fast5 files
  size_t files_count = 0;
  char **fast5_files = find_fast5_files(directory, recursive, &files_count);
  
  if (!fast5_files || files_count == 0) {
    printf("No Fast5 files found in directory.\n");
    return;
  }
  
  printf("Found %zu Fast5 files:\n\n", files_count);
  
  // Process each file
  for (size_t i = 0; i < files_count; i++) {
    display_fast5_info(fast5_files[i], verbose);
  }
  
  free_file_list(fast5_files, files_count);
}

int main_fast5(int argc, char *argv[]) {
  // Handle help for fast5 subcommand
  if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
    printf("Sequelizer fast5 - Fast5 File Operations\n");
    printf("========================================\n\n");
    printf("Tools for working with Fast5 nanopore data files.\n\n");
    printf("Usage: sequelizer fast5 [command] [options] <input>\n\n");
    printf("Commands:\n");
    printf("  info      Display Fast5 file information (default)\n\n");
    printf("Options:\n");
    printf("  --recursive, -r   Search directories recursively\n");
    printf("  --verbose, -v     Show detailed information\n");
    printf("  --help, -h        Show this help\n\n");
    printf("Examples:\n");
    printf("  sequelizer fast5 data.fast5\n");
    printf("  sequelizer fast5 info --verbose data.fast5\n");
    printf("  sequelizer fast5 /path/to/fast5_files/ --recursive\n");
    return EXIT_SUCCESS;
  }
  
  // Parse arguments
  bool recursive = false;
  bool verbose = false;
  const char *input_path = NULL;
  const char *command = "info"; // Default command
  
  // Simple argument parsing
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--recursive") == 0 || strcmp(argv[i], "-r") == 0) {
      recursive = true;
    } else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
      verbose = true;
    } else if (strcmp(argv[i], "info") == 0) {
      command = "info";
    } else if (argv[i][0] != '-') {
      // First non-option argument is the input path
      if (!input_path) {
        input_path = argv[i];
      }
    }
  }
  
  if (!input_path) {
    printf("Error: No input file or directory specified.\n");
    printf("Use 'sequelizer fast5 --help' for usage information.\n");
    return EXIT_FAILURE;
  }
  
  // Check if input exists
  struct stat path_stat;
  if (stat(input_path, &path_stat) != 0) {
    printf("Error: Input path does not exist: %s\n", input_path);
    return EXIT_FAILURE;
  }
  
  // Process based on input type
  if (S_ISREG(path_stat.st_mode)) {
    // Single file
    if (!is_fast5_file(input_path)) {
      printf("Error: Input file is not a Fast5 file: %s\n", input_path);
      return EXIT_FAILURE;
    }
    display_fast5_info(input_path, verbose);
  } else if (S_ISDIR(path_stat.st_mode)) {
    // Directory
    process_directory(input_path, recursive, verbose);
  } else {
    printf("Error: Input path is neither a file nor a directory: %s\n", input_path);
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}