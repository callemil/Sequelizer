// **********************************************************************
// sequelizer_fast5.c
// **********************************************************************
// S Magierowski Jul 26 2025
//
// Fast5 file operations for Sequelizer
// 
// ./sequelizer fast5 /Users/seb/Documents/GitHub/SquiggleFilter/data/lambda/fast5/ --recursive --verbose
// ./sequelizer fast5 /Users/seb/Documents/GitHub/slow5tools/test/data --recursive --verbose 

#include "sequelizer_fast5.h"
#include "core/fast5_io.h"
#include "core/fast5_utils.h"
#include "core/fast5_stats.h"
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

// Helper function to display single file info using pre-loaded metadata (avoids re-reading)
static void display_single_file_info_from_metadata(fast5_metadata_t *metadata, int count, 
                                                   const char *filename, bool verbose) {
  printf("Fast5 File: %s\n", filename);
  printf("=====================================\n");
  printf("File size: %.2f MB\n", get_file_size_mb(filename));
  
  if (count <= 0) {
    printf("Error: Could not read metadata from file\n\n");
    return;
  }
  
  // File format and basic info
  printf("Format: %s\n", metadata[0].is_multi_read ? "Multi-read" : "Single-read");
  printf("Reads: %d\n", count);
  
  if (count > 0) {
    printf("Sample rate: %.0f Hz\n", metadata[0].sample_rate);
  }
  
  // Calculate statistics from stored metadata
  uint32_t total_signal_length = 0;
  uint32_t min_signal_length = UINT32_MAX;
  uint32_t max_signal_length = 0;
  double total_duration = 0.0;
  
  for (int i = 0; i < count; i++) {
    uint32_t length = metadata[i].signal_length;
    total_signal_length += length;
    
    if (length > 0) {
      if (length < min_signal_length) min_signal_length = length;
      if (length > max_signal_length) max_signal_length = length;
    }
    
    if (metadata[i].sample_rate > 0) {
      total_duration += (double)metadata[i].duration / metadata[i].sample_rate;
    }
  }
  
  if (count > 0 && min_signal_length == UINT32_MAX) min_signal_length = 0;
  
  printf("Signal statistics:\n");
  printf("  Total samples: %u\n", total_signal_length);
  printf("  Average length: %.0f samples\n", count > 0 ? (double)total_signal_length / count : 0.0);
  printf("  Range: %u - %u samples\n", min_signal_length, max_signal_length);
  printf("  Total duration: %.1f seconds\n", total_duration);
  printf("  Average duration: %.1f seconds\n", count > 0 ? total_duration / count : 0.0);
  
  if (verbose) {
    printf("\nDetailed read information:\n");
    for (int i = 0; i < count; i++) {
      printf("  Read %d: %s (%u samples)\n", i + 1,
             metadata[i].read_id ? metadata[i].read_id : "unknown",
             metadata[i].signal_length);
    }
  } else if (count <= 3) {
    printf("\nRead details:\n");
    for (int i = 0; i < count; i++) {
      printf("  Read %d: %s (%u samples)\n", i + 1, 
             metadata[i].read_id ? metadata[i].read_id : "unknown", 
             metadata[i].signal_length);
    }
  } else {
    printf("\nShowing first 3 reads (use --verbose for all):\n");
    for (int i = 0; i < 3; i++) {
      printf("  Read %d: %s (%u samples)\n", i + 1, 
             metadata[i].read_id ? metadata[i].read_id : "unknown", 
             metadata[i].signal_length);
    }
    printf("  ... and %d more reads\n", count - 3);
  }
  
  printf("\n");
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
  size_t file_count = 0;
  char **fast5_files = find_fast5_files(arguments.input_path, arguments.recursive, &file_count);
  
  // Handle case where no Fast5 files are found
  if (!fast5_files || file_count == 0) {
    printf("No Fast5 files found.\n");
    return EXIT_SUCCESS;
  }
  
  // ========================================================================
  // STEP 3: INITIALIZE TIMING AND DATA STRUCTURES
  // ========================================================================
  // Start timing for performance measurement
  struct timeval start_time, end_time;
  gettimeofday(&start_time, NULL);
  
  // Initialize shared arrays for storing results from file processing
  fast5_metadata_t **results = NULL;
  int *results_count = NULL;
  initialize_data_structures(file_count, &results, &results_count);
  
  // ========================================================================
  // STEP 4: PROCESS FILES SEQUENTIALLY WITH PROGRESS TRACKING  
  // ========================================================================
  // Process each file and collect metadata results (single-threaded)
  process_files_sequentially(fast5_files, file_count, results, results_count, arguments.verbose);
  
  // Calculate total processing time for summary
  gettimeofday(&end_time, NULL);
  double processing_time_ms = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) + 
                             ((end_time.tv_usec - start_time.tv_usec) / 1000.0);

  // ========================================================================
  // STEP 5: CALCULATE FAST5 DATASET STATISTICS
  // ========================================================================
  fast5_dataset_statistics_t *stats = calc_fast5_dataset_stats_with_enhancer(results, results_count, fast5_files, file_count, NULL);
  
  // ========================================================================
  // STEP 6: CREATE ANALYSIS SUMMARY
  // ========================================================================
  fast5_analysis_summary_t* summary = calc_analysis_summary_with_enhancer(stats, file_count, processing_time_ms, NULL);

  // ========================================================================
  // STEP 6: OUTPUT RESULTS IN REQUESTED FORMAT
  // ========================================================================
  // Handle special cases (debug mode, single files) and regular output
  if (file_count == 1) {
    // Single file processing - display individual file info
    if (arguments.debug) {
      debug_fast5_file(fast5_files[0]);
    } else {
      display_single_file_info_from_metadata(results[0], results_count[0], fast5_files[0], arguments.verbose);
    }
  } else if (arguments.debug) {
    // Debug mode directory processing (first file only)
    printf("Debug mode: Processing first file found: %s\n\n", fast5_files[0]);
    debug_fast5_file(fast5_files[0]);
  } else {
    // Regular directory processing
    // Show individual file info if verbose mode enabled (using stored metadata)
    if (arguments.verbose) {
      for (size_t i = 0; i < file_count; i++) {
        if (results[i] && results_count[i] > 0) {
          display_single_file_info_from_metadata(results[i], results_count[i], fast5_files[i], arguments.verbose);
        }
      }
    }
    
  //   // Generate and display summary statistics for the entire dataset
  //   create_analysis_summary(results, results_count, fast5_files, file_count, processing_time_ms);
  }
  print_comprehensive_summary_human(summary);
  free_comprehensive_summary(summary);

  // ========================================================================
  // STEP 7: CLEANUP ALL ALLOCATED RESOURCES
  // ========================================================================
  // Free metadata results from all successfully processed files
  for (size_t i = 0; i < file_count; i++) {
    if (results[i] && results_count[i] > 0) {
      free_fast5_metadata(results[i], results_count[i]);
    }
  }
  
  // Free main data structure arrays
  free(results);
  free(results_count);
  free_file_list(fast5_files, file_count);
  
  return EXIT_SUCCESS;
}