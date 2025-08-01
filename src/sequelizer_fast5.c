// **********************************************************************
// sequelizer_fast5.c
// **********************************************************************
// S Magierowski Jul 26 2025
//
// Fast5 file operations for Sequelizer
// 
// ./sequelizer fast5 /Users/seb/Documents/GitHub/SquiggleFilter/data/lambda/fast5/ --recursive --verbose
// ./sequelizer fast5 /Users/seb/Documents/GitHub/slow5tools/test/data --recursive --verbose 

#include "../include/sequelizer.h"
#include "sequelizer_fast5.h"
#include "core/fast5_io.h"
#include <string.h>
#include <sys/stat.h> // retrieve files & directory status information
#include <stdint.h>

// Helper function to display file information
static void display_fast5_info(const char *filename, bool verbose) {
  printf("Fast5 File: %s\n", filename);
  printf("=====================================\n");
  
  // Add basic file checks
  struct stat file_stat;
  if (stat(filename, &file_stat) != 0) {
    printf("Error: Cannot access file\n\n");
    return;
  }
  
  if (verbose) {
    printf("File size: %.2f MB\n", (double)file_stat.st_size / (1024.0 * 1024.0));
  }
  
  // Get metadata for all reads in the file
  size_t metadata_count = 0;
  fast5_metadata_t *metadata = read_fast5_metadata(filename, &metadata_count);
  
  if (!metadata || metadata_count == 0) {
    printf("Error: Could not read metadata from file\n");
    if (verbose) {
      printf("Debug: File exists and has size %.2f MB, but metadata extraction failed\n", 
             (double)file_stat.st_size / (1024.0 * 1024.0));
      printf("This may indicate an incompatible Fast5 format or corrupted file\n");
    }
    printf("\n");
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

// Helper function to debug HDF5 file structure
static void debug_fast5_file(const char *filename) {
  printf("Fast5 Debug: %s\n", filename);
  printf("=============================\n");
  
  // Basic file info
  struct stat file_stat;
  if (stat(filename, &file_stat) != 0) {
    printf("Error: Cannot access file\n\n");
    return;
  }
  
  printf("File size: %.2f MB\n", (double)file_stat.st_size / (1024.0 * 1024.0));
  
  // Try to open with HDF5
  printf("Attempting HDF5 open...\n");
  
  hid_t file_id = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
  if (file_id < 0) {
    printf("FAILED: Cannot open as HDF5 file\n");
    printf("This file may be corrupted or not a valid Fast5 file\n\n");
    return;
  }
  
  printf("SUCCESS: HDF5 file opened\n");
  
  // Check for common Fast5 attributes
  printf("\nChecking file attributes:\n");
  
  htri_t file_type_exists = H5Aexists(file_id, "file_type");
  printf("- file_type attribute: %s\n", 
         file_type_exists > 0 ? "EXISTS" : 
         file_type_exists == 0 ? "NOT FOUND" : "ERROR");
  
  htri_t file_version_exists = H5Aexists(file_id, "file_version");
  printf("- file_version attribute: %s\n", 
         file_version_exists > 0 ? "EXISTS" : 
         file_version_exists == 0 ? "NOT FOUND" : "ERROR");
  
  // Check for common groups
  printf("\nChecking common groups:\n");
  
  htri_t raw_reads_exists = H5Lexists(file_id, "/Raw/Reads", H5P_DEFAULT);
  printf("- /Raw/Reads: %s\n", 
         raw_reads_exists > 0 ? "EXISTS" : 
         raw_reads_exists == 0 ? "NOT FOUND" : "ERROR");
  
  // Try to list root level objects
  printf("\nRoot level objects:\n");
  
  hsize_t num_objs;
  if (H5Gget_num_objs(file_id, &num_objs) >= 0) {
    printf("Found %lld root objects:\n", num_objs);
    for (hsize_t i = 0; i < num_objs && i < 10; i++) { // Limit to first 10
      char name[256];
      ssize_t name_size = H5Gget_objname_by_idx(file_id, i, name, sizeof(name));
      if (name_size > 0) {
        H5G_obj_t obj_type = H5Gget_objtype_by_idx(file_id, i);
        const char *type_str = (obj_type == H5G_GROUP) ? "GROUP" : 
                              (obj_type == H5G_DATASET) ? "DATASET" : "OTHER";
        printf("  [%lld] %s (%s)\n", i, name, type_str);
      }
    }
    if (num_objs > 10) {
      printf("  ... and %lld more objects\n", num_objs - 10);
    }
  } else {
    printf("Error: Cannot enumerate root objects\n");
  }
  
  H5Fclose(file_id);
  printf("\n");
}

int main_fast5(int argc, char *argv[]) {
  // Handle help for fast5 subcommand
  if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
    printf("Sequelizer fast5 - Fast5 File Operations\n");
    printf("========================================\n\n");
    printf("Tools for working with Fast5 nanopore data files.\n\n");
    printf("Usage: sequelizer fast5 [command] [options] <input>\n\n");
    printf("Commands:\n");
    printf("  info      Display Fast5 file information (default)\n");
    printf("  debug     Show detailed HDF5 structure for debugging\n\n");
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
    } else if (strcmp(argv[i], "debug") == 0) {
      command = "debug";
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
  
  // Process based on input type and command
  if (S_ISREG(path_stat.st_mode)) {
    // Single file
    if (!is_fast5_file(input_path)) {
      printf("Error: Input file is not a Fast5 file: %s\n", input_path);
      return EXIT_FAILURE;
    }
    
    if (strcmp(command, "debug") == 0) {
      debug_fast5_file(input_path);
    } else {
      display_fast5_info(input_path, verbose);
    }
  } else if (S_ISDIR(path_stat.st_mode)) {
    // Directory - debug mode only processes first file found
    if (strcmp(command, "debug") == 0) {
      size_t files_count = 0;
      char **fast5_files = find_fast5_files(input_path, recursive, &files_count);
      
      if (!fast5_files || files_count == 0) {
        printf("No Fast5 files found in directory.\n");
        return EXIT_FAILURE;
      }
      
      printf("Debug mode: Processing first file found: %s\n\n", fast5_files[0]);
      debug_fast5_file(fast5_files[0]);
      
      free_file_list(fast5_files, files_count);
    } else {
      process_directory(input_path, recursive, verbose);
    }
  } else {
    printf("Error: Input path is neither a file nor a directory: %s\n", input_path);
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}