// **********************************************************************
// core/fast5_io.c - Fast5 File I/O Operations for Sequelizer
// **********************************************************************
// Sebastian Claudiusz Magierowski Aug 3 2025
//
// Migrated from ciren Fast5 I/O operations including:
// - Fast5 file discovery and pattern matching  
// - Fast5 metadata extraction (single-read and multi-read formats)
// - Fast5 signal data extraction
// Used by sequelizer fast5 subcommand and future signal processing.

#include "fast5_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hdf5.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <err.h>

// **********************************************************************
// Fast5 File Discovery and Validation Functions
// **********************************************************************

// Pattern matching for fast5 extension 
bool is_fast5_file(const char *filename) {
  if (!filename) return false;
  size_t len = strlen(filename);
  return len >= 6 && strcmp(filename + len - 6, ".fast5") == 0;
}

// Enhanced file validation functions (ported from ciren)
bool file_is_accessible(const char *filename) {
  struct stat file_stat;
  return (stat(filename, &file_stat) == 0 && S_ISREG(file_stat.st_mode));
}

bool is_likely_fast5_file(const char *filename) {
  if (!filename) return false;
  size_t len = strlen(filename);
  return len >= 6 && strcmp(filename + len - 6, ".fast5") == 0;
}

bool is_valid_hdf5_file(const char *filename) {
  // Suppress HDF5 error messages temporarily
  H5E_auto2_t old_func;
  void *old_client_data;
  H5Eget_auto2(H5E_DEFAULT, &old_func, &old_client_data);
  H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
  
  // Simple HDF5 file validation
  hid_t file_id = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
  bool is_valid = (file_id >= 0);
  
  if (is_valid) {
    H5Fclose(file_id);
  }
  
  // Restore HDF5 error reporting
  H5Eset_auto2(H5E_DEFAULT, old_func, old_client_data);
  
  return is_valid;
}

bool has_fast5_structure(const char *filename) {
  // Create thread-local error stack for thread safety
  hid_t error_stack = H5Ecreate_stack();
  if (error_stack >= 0) {
    H5Eset_auto2(error_stack, NULL, NULL);
  }
  
  hid_t file_id = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
  if (file_id < 0) {
    if (error_stack >= 0) {
      H5Eclose_stack(error_stack);
    }
    return false;
  }
  
  bool has_structure = false;
  
  // Check for multi-read format indicators
  htri_t attr_exists = H5Aexists(file_id, "file_type");
  if (attr_exists > 0) {
    has_structure = true;
  } else {
    // Check for read_ groups at root level
    hsize_t num_objs;
    if (H5Gget_num_objs(file_id, &num_objs) >= 0 && num_objs > 0) {
      for (hsize_t i = 0; i < num_objs && i < 5; i++) {
        char obj_name[256];
        if (H5Gget_objname_by_idx(file_id, i, obj_name, sizeof(obj_name)) >= 0) {
          if (strncmp(obj_name, "read_", 5) == 0) {
            has_structure = true;
            break;
          }
        }
      }
    }
    
    // Check for single-read format
    if (!has_structure) {
      htri_t exists = H5Lexists(file_id, "/Raw/Reads", H5P_DEFAULT);
      if (exists > 0) {
        has_structure = true;
      }
    }
  }
  
  H5Fclose(file_id);
  
  // Clean up thread-local error stack
  if (error_stack >= 0) {
    H5Eclose_stack(error_stack);
  }
  
  return has_structure;
}

// Recursive directory traversal
char** find_fast5_files_recursive(const char *directory, size_t *count) {
  DIR *dir;
  struct dirent *entry;
  struct stat file_stat;
  char path[PATH_MAX];
  char **files = NULL;
  size_t files_capacity = 0;
  *count = 0;
  
  dir = opendir(directory);
  if (!dir) {
    warnx("Cannot open directory: %s", directory);
    return NULL;
  }
  
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.') continue; // Skip hidden files and . ..
    
    snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);
    
    if (stat(path, &file_stat) != 0) {
      warnx("Cannot stat file: %s", path);
      continue;
    }
    
    if (S_ISDIR(file_stat.st_mode)) {
      // Recursively search subdirectories
      size_t subdir_count;
      char **subdir_files = find_fast5_files_recursive(path, &subdir_count);
      
      if (subdir_files && subdir_count > 0) {
        // Ensure capacity is sufficient for new files
        size_t new_total = *count + subdir_count;
        if (new_total > files_capacity) {
          files_capacity = new_total;
          files = realloc(files, files_capacity * sizeof(char*));
          if (!files) {
            errx(EXIT_FAILURE, "Memory allocation failed");
          }
        }
        
        // Copy subdirectory files to main array
        for (size_t i = 0; i < subdir_count; i++) {
          files[*count + i] = subdir_files[i];
        }
        *count += subdir_count;
        free(subdir_files); // Free the array, but not the strings
      }
    } else if (S_ISREG(file_stat.st_mode) && is_fast5_file(entry->d_name)) {
      // Add Fast5 file to list
      if (*count >= files_capacity) {
        files_capacity = files_capacity == 0 ? 16 : files_capacity * 2;
        files = realloc(files, files_capacity * sizeof(char*));
        if (!files) {
          errx(EXIT_FAILURE, "Memory allocation failed");
        }
      }
      
      files[*count] = strdup(path);
      if (!files[*count]) {
        errx(EXIT_FAILURE, "Memory allocation failed");
      }
      (*count)++;
    }
  }
  
  closedir(dir);
  return files;
}

// Main discovery function handling files/directories
char** find_fast5_files(const char *input_path, bool recursive, size_t *count) {
  struct stat path_stat;
  *count = 0;
  
  // Check if input path exists
  if (stat(input_path, &path_stat) != 0) {
    errx(EXIT_FAILURE, "Input path does not exist: %s", input_path);
  }
  
  if (S_ISREG(path_stat.st_mode)) {
    // Single file
    if (is_fast5_file(input_path)) {
      char **files = malloc(sizeof(char*));
      if (!files) {
        errx(EXIT_FAILURE, "Memory allocation failed");
      }
      files[0] = strdup(input_path);
      if (!files[0]) {
        errx(EXIT_FAILURE, "Memory allocation failed");
      }
      *count = 1;
      return files;
    } else {
      errx(EXIT_FAILURE, "Input file is not a Fast5 file: %s", input_path);
    }
  } else if (S_ISDIR(path_stat.st_mode)) {
    // Directory
    if (recursive) {
      return find_fast5_files_recursive(input_path, count);
    } else {
      // Non-recursive directory search
      DIR *dir;
      struct dirent *entry;
      char **files = NULL;
      size_t files_capacity = 0;
      
      dir = opendir(input_path);
      if (!dir) {
        errx(EXIT_FAILURE, "Cannot open directory: %s", input_path);
      }
      
      while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue; // Skip hidden files
        
        if (is_fast5_file(entry->d_name)) {
          if (*count >= files_capacity) {
            files_capacity = files_capacity == 0 ? 16 : files_capacity * 2;
            files = realloc(files, files_capacity * sizeof(char*));
            if (!files) {
              errx(EXIT_FAILURE, "Memory allocation failed");
            }
          }
          
          char path[PATH_MAX];
          snprintf(path, sizeof(path), "%s/%s", input_path, entry->d_name);
          files[*count] = strdup(path);
          if (!files[*count]) {
            errx(EXIT_FAILURE, "Memory allocation failed");
          }
          (*count)++;
        }
      }
      
      closedir(dir);
      return files;
    }
  } else {
    errx(EXIT_FAILURE, "Input path is neither a file nor a directory: %s", input_path);
  }
  
  return NULL;
}

// Memory cleanup
void free_file_list(char **files, size_t count) {
  if (files) {
    for (size_t i = 0; i < count; i++) {
      free(files[i]);
    }
    free(files);
  }
}

// **********************************************************************
// Fast5 File Reading Functions - Helper Functions
// **********************************************************************

// Helper function to read string attribute
static char* read_string_attribute(hid_t obj_id, const char *attr_name) {
  hid_t attr_id = H5Aopen(obj_id, attr_name, H5P_DEFAULT);
  if (attr_id < 0) return NULL;
  
  hid_t type_id = H5Aget_type(attr_id);
  size_t size = H5Tget_size(type_id);
  
  char *buffer = malloc(size + 1);
  if (!buffer) {
    H5Tclose(type_id);
    H5Aclose(attr_id);
    return NULL;
  }
  
  if (H5Aread(attr_id, type_id, buffer) < 0) {
    free(buffer);
    buffer = NULL;
  } else {
    buffer[size] = '\0';
  }
  
  H5Tclose(type_id);
  H5Aclose(attr_id);
  return buffer;
}

// Helper function to read uint32 attribute
static bool read_uint32_attribute(hid_t obj_id, const char *attr_name, uint32_t *value) {
  hid_t attr_id = H5Aopen(obj_id, attr_name, H5P_DEFAULT);
  if (attr_id < 0) return false;
  
  bool success = (H5Aread(attr_id, H5T_NATIVE_UINT32, value) >= 0);
  H5Aclose(attr_id);
  return success;
}

// Helper function to read double attribute
static bool read_double_attribute(hid_t obj_id, const char *attr_name, double *value) {
  hid_t attr_id = H5Aopen(obj_id, attr_name, H5P_DEFAULT);
  if (attr_id < 0) return false;
  
  bool success = (H5Aread(attr_id, H5T_NATIVE_DOUBLE, value) >= 0);
  H5Aclose(attr_id);
  return success;
}

// Helper function to get signal dataset dimensions
static hsize_t get_signal_length(hid_t signal_dataset_id) {
  hid_t space_id = H5Dget_space(signal_dataset_id);
  if (space_id < 0) return 0;
  
  hsize_t dims[1];
  int ndims = H5Sget_simple_extent_dims(space_id, dims, NULL);
  H5Sclose(space_id);
  
  return (ndims == 1) ? dims[0] : 0;
}

// **********************************************************************
// Fast5 Metadata Reading Functions
// **********************************************************************

// Free Fast5 metadata
void free_fast5_metadata(fast5_metadata_t *metadata, size_t count) {
  if (!metadata) return;
  
  for (size_t i = 0; i < count; i++) {
    free(metadata[i].read_id);
    free(metadata[i].file_path);
    // Only free if it exists (safe for both basic and enhanced)
    free(metadata[i].compression_method);
    free(metadata[i].run_id);
    free(metadata[i].channel_number);
  }
  free(metadata);
}

// **********************************************************************
// Metadata Enhancer Functions
// **********************************************************************

// Enhancer function to extract calibration parameters from Fast5 files
void extract_calibration_parameters(hid_t file_id, hid_t signal_dataset_id, fast5_metadata_t *metadata) {
  // Initialize calibration fields
  metadata->offset = 0.0;
  metadata->range = 0.0;
  metadata->digitisation = 0.0;
  metadata->calibration_available = false;
  
  if (signal_dataset_id < 0) return;
  
  // Get the dataset path to determine read location
  char obj_name[256];
  ssize_t name_len = H5Iget_name(signal_dataset_id, obj_name, sizeof(obj_name));
  if (name_len <= 0) return;
  
  hid_t channel_group_id = -1;
  
  if (strstr(obj_name, "/Raw/Reads/")) {
    // Single-read format: /Raw/Reads/Read_xxx/Signal -> /UniqueGlobalKey/channel_id
    channel_group_id = H5Gopen2(file_id, "/UniqueGlobalKey/channel_id", H5P_DEFAULT);
  } else if (strstr(obj_name, "/Raw/Signal")) {
    // Multi-read format: /read_<UUID>/Raw/Signal -> /read_<UUID>/channel_id
    char channel_path[512];
    char *read_part = strstr(obj_name, "/read_");
    if (read_part) {
      char *raw_part = strstr(read_part, "/Raw");
      if (raw_part) {
        size_t read_len = raw_part - obj_name;
        snprintf(channel_path, sizeof(channel_path), "%.*s/channel_id", (int)read_len, obj_name);
        channel_group_id = H5Gopen2(file_id, channel_path, H5P_DEFAULT);
      }
    }
  }
  
  if (channel_group_id >= 0) {
    bool got_offset = false, got_range = false, got_digitisation = false;
    
    // Extract offset
    hid_t offset_attr = H5Aopen(channel_group_id, "offset", H5P_DEFAULT);
    if (offset_attr >= 0) {
      if (H5Aread(offset_attr, H5T_NATIVE_DOUBLE, &metadata->offset) >= 0) {
        got_offset = true;
      }
      H5Aclose(offset_attr);
    }
    
    // Extract range
    hid_t range_attr = H5Aopen(channel_group_id, "range", H5P_DEFAULT);
    if (range_attr >= 0) {
      if (H5Aread(range_attr, H5T_NATIVE_DOUBLE, &metadata->range) >= 0) {
        got_range = true;
      }
      H5Aclose(range_attr);
    }
    
    // Extract digitisation
    hid_t digitisation_attr = H5Aopen(channel_group_id, "digitisation", H5P_DEFAULT);
    if (digitisation_attr >= 0) {
      if (H5Aread(digitisation_attr, H5T_NATIVE_DOUBLE, &metadata->digitisation) >= 0) {
        got_digitisation = true;
      }
      H5Aclose(digitisation_attr);
    }
    
    // Mark calibration as available if we got all three parameters
    metadata->calibration_available = got_offset && got_range && got_digitisation;
    
    H5Gclose(channel_group_id);
  }
}


// **********************************************************************
// Fast5 Metadata Reading Functions (ENHANCED)
// **********************************************************************
// Enhanced single-read metadata function with enhancer support
static fast5_metadata_t* read_single_read_metadata_with_enhancer(hid_t file_id, const char *filename, size_t *count, metadata_enhancer_t enhancer) {
  *count = 0;
  
  // Suppress HDF5 error messages temporarily
  H5E_auto2_t old_func;
  void *old_client_data;
  H5Eget_auto2(H5E_DEFAULT, &old_func, &old_client_data);
  H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
  
  // Check if /Raw/Reads group exists
  htri_t exists = H5Lexists(file_id, "/Raw/Reads", H5P_DEFAULT);
  
  // Restore HDF5 error reporting
  H5Eset_auto2(H5E_DEFAULT, old_func, old_client_data);
  
  if (exists <= 0) return NULL;
  
  hid_t reads_group_id = H5Gopen2(file_id, "/Raw/Reads", H5P_DEFAULT);
  if (reads_group_id < 0) return NULL;
  
  // Get number of reads in the group
  hsize_t num_objs;
  if (H5Gget_num_objs(reads_group_id, &num_objs) < 0) {
    H5Gclose(reads_group_id);
    return NULL;
  }
  
  if (num_objs == 0) {
    H5Gclose(reads_group_id);
    return NULL;
  }
  
  fast5_metadata_t *metadata = calloc(num_objs, sizeof(fast5_metadata_t));
  if (!metadata) {
    H5Gclose(reads_group_id);
    return NULL;
  }
  
  // Process each read
  for (hsize_t i = 0; i < num_objs; i++) {
    char read_name[256];
    if (H5Gget_objname_by_idx(reads_group_id, i, read_name, sizeof(read_name)) < 0) continue;
    
    char read_path[512];
    snprintf(read_path, sizeof(read_path), "/Raw/Reads/%s", read_name);
    
    hid_t read_group_id = H5Gopen2(file_id, read_path, H5P_DEFAULT);
    if (read_group_id < 0) continue;
    
    // Initialize metadata
    metadata[*count].file_path = strdup(filename);
    metadata[*count].is_multi_read = false;
    
    // Read attributes
    metadata[*count].read_id = read_string_attribute(read_group_id, "read_id");
    read_uint32_attribute(read_group_id, "duration", &metadata[*count].duration);
    read_uint32_attribute(read_group_id, "read_number", &metadata[*count].read_number);
    
    // Get signal length from Signal dataset
    hid_t signal_dataset_id = H5Dopen2(read_group_id, "Signal", H5P_DEFAULT);
    if (signal_dataset_id >= 0) {
      metadata[*count].signal_length = (uint32_t)get_signal_length(signal_dataset_id);
      
      // *** CALL ENHANCER HERE - while signal dataset is open ***
      if (enhancer) {
        enhancer(file_id, signal_dataset_id, &metadata[*count]);
      }
      
      H5Dclose(signal_dataset_id);
    }
    
    H5Gclose(read_group_id);
    (*count)++;
  }
  
  // Get sample rate from channel_id group
  hid_t channel_group_id = H5Gopen2(file_id, "/UniqueGlobalKey/channel_id", H5P_DEFAULT);
  if (channel_group_id >= 0) {
    double sample_rate;
    if (read_double_attribute(channel_group_id, "sampling_rate", &sample_rate)) {
      for (size_t i = 0; i < *count; i++) {
        metadata[i].sample_rate = sample_rate;
      }
    }
    H5Gclose(channel_group_id);
  }
  
  H5Gclose(reads_group_id);
  return metadata;
}

// Enhanced multi-read metadata function with enhancer support
static fast5_metadata_t* read_multi_read_metadata_with_enhancer(hid_t file_id, const char *filename, size_t *count, metadata_enhancer_t enhancer) {
  *count = 0;
  
  // Get number of root-level objects
  hsize_t num_objs;
  if (H5Gget_num_objs(file_id, &num_objs) < 0) return NULL;
  
  // Count read_ groups
  size_t read_count = 0;
  for (hsize_t i = 0; i < num_objs; i++) {
    char obj_name[256];
    if (H5Gget_objname_by_idx(file_id, i, obj_name, sizeof(obj_name)) < 0) continue;
    
    if (strncmp(obj_name, "read_", 5) == 0) {
      read_count++;
    }
  }
  
  if (read_count == 0) return NULL;
  
  fast5_metadata_t *metadata = calloc(read_count, sizeof(fast5_metadata_t));
  if (!metadata) return NULL;
  
  // Process each read_ group
  for (hsize_t i = 0; i < num_objs; i++) {
    char obj_name[256];
    if (H5Gget_objname_by_idx(file_id, i, obj_name, sizeof(obj_name)) < 0) continue;
    
    if (strncmp(obj_name, "read_", 5) != 0) continue;
    
    hid_t read_group_id = H5Gopen2(file_id, obj_name, H5P_DEFAULT);
    if (read_group_id < 0) continue;
    
    // Open Raw subgroup
    hid_t raw_group_id = H5Gopen2(read_group_id, "Raw", H5P_DEFAULT);
    if (raw_group_id < 0) {
      H5Gclose(read_group_id);
      continue;
    }
    
    // Initialize metadata
    metadata[*count].file_path = strdup(filename);
    metadata[*count].is_multi_read = true;
    
    // Read attributes from Raw group
    metadata[*count].read_id = read_string_attribute(raw_group_id, "read_id");
    read_uint32_attribute(raw_group_id, "duration", &metadata[*count].duration);
    read_uint32_attribute(raw_group_id, "read_number", &metadata[*count].read_number);
    
    // Get signal length from Signal dataset
    hid_t signal_dataset_id = H5Dopen2(raw_group_id, "Signal", H5P_DEFAULT);
    if (signal_dataset_id >= 0) {
      metadata[*count].signal_length = (uint32_t)get_signal_length(signal_dataset_id);
      
      // *** CALL ENHANCER HERE - while signal dataset is open ***
      if (enhancer) {
        enhancer(file_id, signal_dataset_id, &metadata[*count]);
      }
      
      H5Dclose(signal_dataset_id);
    }
    
    // Get sample rate from channel_id group
    hid_t channel_group_id = H5Gopen2(read_group_id, "channel_id", H5P_DEFAULT);
    if (channel_group_id >= 0) {
      read_double_attribute(channel_group_id, "sampling_rate", &metadata[*count].sample_rate);
      H5Gclose(channel_group_id);
    }
    
    H5Gclose(raw_group_id);
    H5Gclose(read_group_id);
    (*count)++;
  }
  
  return metadata;
}

// Main function to read Fast5 metadata with enhancer support
fast5_metadata_t* read_fast5_metadata_with_enhancer(const char *filename, size_t *metadata_count, metadata_enhancer_t enhancer) {
  if (!filename || !metadata_count) return NULL;
  
  *metadata_count = 0;
  
  // Suppress HDF5 error messages temporarily
  H5E_auto2_t old_func;
  void *old_client_data;
  H5Eget_auto2(H5E_DEFAULT, &old_func, &old_client_data);
  H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
  
  // Open the Fast5 file
  hid_t file_id = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
  
  // Restore HDF5 error reporting
  H5Eset_auto2(H5E_DEFAULT, old_func, old_client_data);
  
  if (file_id < 0) {
    warnx("Failed to open Fast5 file: %s", filename);
    return NULL;
  }
  
  fast5_metadata_t *metadata = NULL;
  
  // Format detection (same logic as original)
  bool is_multi_read = false;
  
  // Check for file_type attribute first
  htri_t attr_exists = H5Aexists(file_id, "file_type");
  if (attr_exists > 0) {
    is_multi_read = true;
  } else {
    // No file_type attribute - check for read_* groups at root level
    hsize_t num_objs;
    if (H5Gget_num_objs(file_id, &num_objs) >= 0 && num_objs > 0) {
      // Check first few objects for read_ pattern
      for (hsize_t i = 0; i < num_objs && i < 5; i++) {
        char obj_name[256];
        if (H5Gget_objname_by_idx(file_id, i, obj_name, sizeof(obj_name)) >= 0) {
          if (strncmp(obj_name, "read_", 5) == 0) {
            is_multi_read = true;
            break;
          }
        }
      }
    }
  }
  
  // Call the appropriate enhanced helper function
  if (is_multi_read) {
    metadata = read_multi_read_metadata_with_enhancer(file_id, filename, metadata_count, enhancer);
  } else {
    metadata = read_single_read_metadata_with_enhancer(file_id, filename, metadata_count, enhancer);
  }
  
  H5Fclose(file_id);
  return metadata;
}

// **********************************************************************
// Fast5 Signal Extraction Functions
// **********************************************************************
// Extract signal data from single-read Fast5 format
static float* read_single_read_signal(hid_t file_id, const char *read_id, size_t *signal_length) {
  *signal_length = 0;
  
  // Suppress HDF5 error messages temporarily
  H5E_auto2_t old_func;
  void *old_client_data;
  H5Eget_auto2(H5E_DEFAULT, &old_func, &old_client_data);
  H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
  
  // Check if /Raw/Reads group exists
  htri_t exists = H5Lexists(file_id, "/Raw/Reads", H5P_DEFAULT);
  
  // Restore HDF5 error reporting
  H5Eset_auto2(H5E_DEFAULT, old_func, old_client_data);
  
  if (exists <= 0) return NULL;
  
  hid_t reads_group_id = H5Gopen2(file_id, "/Raw/Reads", H5P_DEFAULT);
  if (reads_group_id < 0) return NULL;
  
  // Get number of reads in the group
  hsize_t num_objs;
  if (H5Gget_num_objs(reads_group_id, &num_objs) < 0) {
    H5Gclose(reads_group_id);
    return NULL;
  }
  
  float *signal = NULL;
  
  // Search for the read
  for (hsize_t i = 0; i < num_objs; i++) {
    char read_name[256];
    if (H5Gget_objname_by_idx(reads_group_id, i, read_name, sizeof(read_name)) < 0) continue;
    
    char read_path[512];
    snprintf(read_path, sizeof(read_path), "/Raw/Reads/%s", read_name);
    
    hid_t read_group_id = H5Gopen2(file_id, read_path, H5P_DEFAULT);
    if (read_group_id < 0) continue;
    
    // Check if this is the read we want (if read_id is specified)
    if (read_id) {
      char *current_read_id = read_string_attribute(read_group_id, "read_id");
      if (!current_read_id || strcmp(current_read_id, read_id) != 0) {
        free(current_read_id);
        H5Gclose(read_group_id);
        continue;
      }
      free(current_read_id);
    }
    
    // Open Signal dataset
    hid_t signal_dataset_id = H5Dopen2(read_group_id, "Signal", H5P_DEFAULT);
    if (signal_dataset_id >= 0) {
      hsize_t dims[1];
      hid_t space_id = H5Dget_space(signal_dataset_id);
      int ndims = H5Sget_simple_extent_dims(space_id, dims, NULL);
      H5Sclose(space_id);
      
      if (ndims == 1 && dims[0] > 0) {
        *signal_length = dims[0];
        signal = malloc(dims[0] * sizeof(float));
        if (signal) {
          if (H5Dread(signal_dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, signal) < 0) {
            free(signal);
            signal = NULL;
            *signal_length = 0;
          }
        }
      }
      H5Dclose(signal_dataset_id);
    }
    
    H5Gclose(read_group_id);
    
    // If we found a signal, break (unless read_id was NULL, in which case we take the first one)
    if (signal) break;
  }
  
  H5Gclose(reads_group_id);
  return signal;
}

// Extract signal data from multi-read Fast5 format
static float* read_multi_read_signal(hid_t file_id, const char *read_id, size_t *signal_length) {
  *signal_length = 0;
  
  // Get number of root-level objects
  hsize_t num_objs;
  if (H5Gget_num_objs(file_id, &num_objs) < 0) return NULL;
  
  float *signal = NULL;
  
  // Process each read_ group
  for (hsize_t i = 0; i < num_objs; i++) {
    char obj_name[256];
    if (H5Gget_objname_by_idx(file_id, i, obj_name, sizeof(obj_name)) < 0) continue;
    
    if (strncmp(obj_name, "read_", 5) != 0) continue;
    
    hid_t read_group_id = H5Gopen2(file_id, obj_name, H5P_DEFAULT);
    if (read_group_id < 0) continue;
    
    // Open Raw subgroup
    hid_t raw_group_id = H5Gopen2(read_group_id, "Raw", H5P_DEFAULT);
    if (raw_group_id < 0) {
      H5Gclose(read_group_id);
      continue;
    }
    
    // Check if this is the read we want (if read_id is specified)
    if (read_id) {
      char *current_read_id = read_string_attribute(raw_group_id, "read_id");
      if (!current_read_id || strcmp(current_read_id, read_id) != 0) {
        free(current_read_id);
        H5Gclose(raw_group_id);
        H5Gclose(read_group_id);
        continue;
      }
      free(current_read_id);
    }
    
    // Open Signal dataset
    hid_t signal_dataset_id = H5Dopen2(raw_group_id, "Signal", H5P_DEFAULT);
    if (signal_dataset_id >= 0) {
      hsize_t dims[1];
      hid_t space_id = H5Dget_space(signal_dataset_id);
      int ndims = H5Sget_simple_extent_dims(space_id, dims, NULL);
      H5Sclose(space_id);
      
      if (ndims == 1 && dims[0] > 0) {
        *signal_length = dims[0];
        signal = malloc(dims[0] * sizeof(float));
        if (signal) {
          if (H5Dread(signal_dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, signal) < 0) {
            free(signal);
            signal = NULL;
            *signal_length = 0;
          }
        }
      }
      H5Dclose(signal_dataset_id);
    }
    
    H5Gclose(raw_group_id);
    H5Gclose(read_group_id);
    
    // If we found a signal, break (unless read_id was NULL, in which case we take the first one)
    if (signal) break;
  }
  
  return signal;
}

// Main function to read Fast5 signal data
float* read_fast5_signal(const char *filename, const char *read_id, size_t *signal_length) {
  if (!filename || !signal_length) return NULL;
  
  *signal_length = 0;
  
  // Suppress HDF5 error messages temporarily
  H5E_auto2_t old_func;
  void *old_client_data;
  H5Eget_auto2(H5E_DEFAULT, &old_func, &old_client_data);
  H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
  
  // Open the Fast5 file
  hid_t file_id = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
  
  // Restore HDF5 error reporting
  H5Eset_auto2(H5E_DEFAULT, old_func, old_client_data);
  
  if (file_id < 0) {
    warnx("Failed to open Fast5 file: %s", filename);
    return NULL;
  }
  
  float *signal = NULL;
  
  // Improved format detection - check multiple indicators
  bool is_multi_read = false;
  
  // Check for file_type attribute first
  htri_t attr_exists = H5Aexists(file_id, "file_type");
  if (attr_exists > 0) {
    is_multi_read = true;
  } else {
    // No file_type attribute - check for read_* groups at root level
    hsize_t num_objs;
    if (H5Gget_num_objs(file_id, &num_objs) >= 0 && num_objs > 0) {
      // Check first few objects for read_ pattern
      for (hsize_t i = 0; i < num_objs && i < 5; i++) {
        char obj_name[256];
        if (H5Gget_objname_by_idx(file_id, i, obj_name, sizeof(obj_name)) >= 0) {
          if (strncmp(obj_name, "read_", 5) == 0) {
            is_multi_read = true;
            break;
          }
        }
      }
    }
  }
  
  if (is_multi_read) {
    signal = read_multi_read_signal(file_id, read_id, signal_length);
  } else {
    signal = read_single_read_signal(file_id, read_id, signal_length);
  }
  
  H5Fclose(file_id);
  return signal;
}

// Free Fast5 signal data
void free_fast5_signal(float *signal) {
  if (signal) {
    free(signal);
  }
}
