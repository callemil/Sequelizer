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
    printf(" analyzing Fast5 files");
  }
  
  fflush(stdout);
}

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
    printf("File size: %.2f MB\n", get_file_size_mb(filename));
  }
  
  // Get metadata for all reads in the file
  size_t metadata_count = 0;
  fast5_metadata_t *metadata = read_fast5_metadata(filename, &metadata_count);
  
  if (!metadata || metadata_count == 0) {
    printf("Error: Could not read metadata from file\n");
    if (verbose) {
      printf("Debug: File exists and has size %.2f MB, but metadata extraction failed\n", 
             get_file_size_mb(filename));
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
    double file_size_mb = get_file_size_mb(filename);
    printf("  File size: %.2f MB\n", file_size_mb);
    if (total_signal_length > 0) {
      struct stat file_stat;
      if (stat(filename, &file_stat) == 0) {
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
      printf("  Read %zu: %s\n", i + 1, metadata[i].read_id ? metadata[i].read_id : "unknown");
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
      printf("  Read %zu: %s (%u samples)\n", i + 1, metadata[i].read_id ? metadata[i].read_id : "unknown", metadata[i].signal_length);
    }
  } else {
    printf("\nShowing first 3 reads (use --verbose for all):\n");
    for (size_t i = 0; i < 3; i++) {
      printf("  Read %zu: %s (%u samples)\n", i + 1, metadata[i].read_id ? metadata[i].read_id : "unknown", metadata[i].signal_length);
    }
    printf("  ... and %zu more reads\n", metadata_count - 3);
  }
  
  printf("\n");
  free_fast5_metadata(metadata, metadata_count);
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
  
  printf("File size: %.2f MB\n", get_file_size_mb(filename));
  
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

// **********************************************************************
// Argument Parsing
// **********************************************************************

const char *argp_program_version = "sequelizer fast5 1.0";
const char *argp_program_bug_address = "magierowski@gmail.com";

static char doc[] = "sequelizer fast5 -- Fast5 file analysis and debugging\v"
"EXAMPLES:\n"
"  sequelizer fast5 data.fast5\n"
"  sequelizer fast5 /path/to/fast5_files/ --recursive --verbose\n"
"  sequelizer fast5 debug problematic.fast5";

static char args_doc[] = "INPUT";

static struct argp_option options[] = {
  {"recursive",     'r', 0,         0, "Search directories recursively"},
  {"verbose",       'v', 0,         0, "Show detailed information"},
  {"debug",         'd', 0,         0, "Show detailed HDF5 structure for debugging"},
  {0}
};

struct arguments {
  char *input_path;
  bool recursive;
  bool verbose;
  bool debug;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;
  
  switch (key) {
    case 'r':
      arguments->recursive = true;
      break;
    case 'v':
      arguments->verbose = true;
      break;
    case 'd':
      arguments->debug = true;
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
// Helper Functions for Modular Architecture
// **********************************************************************


// Helper function to initialize data structures for file processing
static void initialize_data_structures(size_t file_count, fast5_metadata_t ***results, int **results_count) {
  *results = calloc(file_count, sizeof(fast5_metadata_t*));
  *results_count = calloc(file_count, sizeof(int));
  
  if (!*results || !*results_count) {
    errx(EXIT_FAILURE, "Memory allocation failed for data structures");
  }
}

// Helper function to process files sequentially with progress tracking
static void process_files_sequentially(char **fast5_files, size_t files_count, 
                                      fast5_metadata_t **results, int *results_count,
                                      bool verbose) {
  // Show initial progress bar
  display_progress(0, (int)files_count, verbose);
  
  // Process each file and collect results
  for (size_t i = 0; i < files_count; i++) {
    // Read metadata for summary calculation
    size_t metadata_count = 0;
    fast5_metadata_t *metadata = read_fast5_metadata_with_enhancer(fast5_files[i], &metadata_count, NULL);
    
    if (metadata && metadata_count > 0) {
      results[i] = metadata;
      results_count[i] = (int)metadata_count;
    } else {
      results[i] = NULL;
      results_count[i] = 0;
    }
    
    // Update progress bar
    display_progress((int)(i + 1), (int)files_count, verbose);
  }
  
  // Complete progress bar and move to next line
  printf("\n\n");
}

// Helper function to create and display analysis summary
static void create_analysis_summary(fast5_metadata_t **results, int *results_count,
                                   char **fast5_files, size_t files_count,
                                   double processing_time_ms) {
  basic_fast5_summary_t *summary = calculate_basic_summary((void**)results, fast5_files, 
                                                          results_count, (int)files_count, 
                                                          processing_time_ms);
  if (summary) {
    print_basic_summary_human(summary);
    free_basic_summary(summary);
  }
}

// **********************************************************************
// Main Function 
// **********************************************************************

int main_fast5(int argc, char *argv[]) {

  // ========================================================================
  // STEP 1: PARSE COMMAND LINE ARGUMENTS AND SET DEFAULTS
  // ========================================================================
  struct arguments arguments;
  
  // Set sensible defaults for all configuration options
  arguments.input_path = NULL;
  arguments.recursive = false;
  arguments.verbose = false;
  arguments.debug = false;
  
  // Parse command line arguments using argp framework
  argp_parse(&argp, argc, argv, 0, 0, &arguments);
  
  // ========================================================================
  // STEP 2: DISCOVER AND ENUMERATE FAST5 FILES
  // ========================================================================
  size_t files_count = 0;
  char **fast5_files = find_fast5_files(arguments.input_path, arguments.recursive, &files_count);
  
  // Handle case where no Fast5 files are found
  if (!fast5_files || files_count == 0) {
    printf("No Fast5 files found.\n");
    return EXIT_SUCCESS;
  }
  
  // ========================================================================
  // STEP 3: HANDLE SPECIAL CASES - DEBUG MODE AND SINGLE FILES
  // ========================================================================
  if (files_count == 1) {
    // STEP 3A: SINGLE FILE PROCESSING
    if (arguments.debug) {
      debug_fast5_file(fast5_files[0]);
      free_file_list(fast5_files, files_count);
      return EXIT_SUCCESS;
    } else {
      display_fast5_info(fast5_files[0], arguments.verbose);
      free_file_list(fast5_files, files_count);
      return EXIT_SUCCESS;
    }
    
  } else if (arguments.debug) {
    // STEP 3B: DEBUG MODE DIRECTORY PROCESSING (first file only)
    printf("Debug mode: Processing first file found: %s\n\n", fast5_files[0]);
    debug_fast5_file(fast5_files[0]);
    free_file_list(fast5_files, files_count);
    return EXIT_SUCCESS;
  }
  
  // ========================================================================
  // STEP 4: DIRECTORY PROCESSING SETUP
  // ========================================================================
  printf("Fast5 Directory Analysis\n");
  printf("========================\n");
  printf("Directory: %s\n", arguments.input_path);
  printf("Recursive: %s\n\n", arguments.recursive ? "yes" : "no");
  printf("Found %zu Fast5 files:\n\n", files_count);
  
  // ========================================================================
  // STEP 5: INITIALIZE TIMING AND DATA STRUCTURES
  // ========================================================================
  // Start timing for performance measurement
  struct timeval start_time, end_time;
  gettimeofday(&start_time, NULL);
  
  // Initialize shared arrays for storing results
  fast5_metadata_t **results = NULL;
  int *results_count = NULL;
  initialize_data_structures(files_count, &results, &results_count);
  
  // ========================================================================
  // STEP 6: PROCESS FILES SEQUENTIALLY WITH PROGRESS TRACKING
  // ========================================================================
  // Process each file and collect metadata results
  process_files_sequentially(fast5_files, files_count, results, results_count, arguments.verbose);
  
  // ========================================================================
  // STEP 7: DISPLAY INDIVIDUAL FILE INFORMATION IF REQUESTED
  // ========================================================================
  // Show individual file info if verbose mode enabled
  if (arguments.verbose) {
    for (size_t i = 0; i < files_count; i++) {
      display_fast5_info(fast5_files[i], arguments.verbose);
    }
  }
  
  // ========================================================================
  // STEP 8: CALCULATE PROCESSING TIME
  // ========================================================================
  // Calculate total processing time for summary
  gettimeofday(&end_time, NULL);
  double processing_time_ms = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) + 
                             ((end_time.tv_usec - start_time.tv_usec) / 1000.0);
  
  // ========================================================================
  // STEP 9: CREATE AND DISPLAY COMPREHENSIVE ANALYSIS SUMMARY
  // ========================================================================
  // Generate and display summary statistics for the entire dataset
  create_analysis_summary(results, results_count, fast5_files, files_count, processing_time_ms);
  
  // ========================================================================
  // STEP 10: CLEANUP ALL ALLOCATED RESOURCES
  // ========================================================================
  // Free metadata results from all successfully processed files
  for (size_t i = 0; i < files_count; i++) {
    if (results[i] && results_count[i] > 0) {
      free_fast5_metadata(results[i], results_count[i]);
    }
  }
  
  // Free main data structure arrays
  free(results);
  free(results_count);
  free_file_list(fast5_files, files_count);
  
  return EXIT_SUCCESS;
}