#ifndef SEQUELIZER_FAST5_IO_H
#define SEQUELIZER_FAST5_IO_H

#include <hdf5.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Fast5 metadata structure
typedef struct {
    char *read_id;
    uint32_t signal_length;
    double sample_rate;
    uint32_t duration;
    uint32_t read_number;
    bool is_multi_read;
    char *file_path;
} fast5_metadata_t;

// Fast5 file discovery functions
bool is_fast5_file(const char *filename);
char** find_fast5_files_recursive(const char *directory, size_t *count);
char** find_fast5_files(const char *input_path, bool recursive, size_t *count);
void free_file_list(char **files, size_t count);

// Fast5 file reading functions
fast5_metadata_t* read_fast5_metadata(const char *filename, size_t *metadata_count);
void free_fast5_metadata(fast5_metadata_t *metadata, size_t count);

// Signal extraction functions
float* read_fast5_signal(const char *filename, const char *read_id, size_t *signal_length);
void free_fast5_signal(float *signal);

#endif // SEQUELIZER_FAST5_IO_H