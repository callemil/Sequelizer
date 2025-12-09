---
layout: default
title: Architecture
---
# Architecture & Design Philosophy

## Why Sequelizer Exists

Built in portable C for custom processors and resource-constrained hardware. 
Generate synthetic nanopore streams. Test real-time analysis on actual edge 
devices. Build the infrastructure for genomics everywhere - not just in 
well-funded labs with server racks.

## Design Principles

- **Portable C foundation** - Target RISC-V, custom accelerators, embedded systems
- **Hardware-agnostic core** - Adapts from no SIMD to GPU acceleration
- **Streaming-first** - Designed for real-time data processing, not batch jobs
- **Simulation-driven** - Test pipelines with synthetic data before hardware deployment

## Fast5 Processing

### In `ciren_fast5.c`
```c
// STEP 3
fast5_metadata_t **results = NULL;
// STEP 4B results are in worker_args
pthread_create(&threads[i], NULL, fast5_worker_thread, &worker_args[i]);
// STEP 5
fast5_dataset_statistics_t *stats = calc_fast5_dataset_stats_with_enhancer(results, results_count, fast5_files, file_count, stats_enhancer);
// STEP 6
fast5_analysis_summary_t* summary = calc_analysis_summary_with_enhancer(stats, file_count, processing_time_ms, summary_enhancer);
```
The `fast5_worker_thread()` function handles the processing of individual Fast5 files in separate threads, ensuring thread-safe access to HDF5 resources through mutex locking.  A key piece of what's inside is
```c
fast5_metadata_t *metadata = read_fast5_metadata_thread_safe(filename, &metadata_count, worker_args->hdf5_mutex); // thread-safe HDF5 access
```
This function is tasked with reading a single file and returning its metadata.  As part of this it gets a lock (hdf5_mutex) before any HDF5 calls, and releases it afterward, ensuring no two threads access HDF5 simultaneously.  And inside `read_fast5_metadata_thread_safe()` we actually deal with our Fast5 file data using the funciton:
```c
fast5_metadata_t *metadata = read_fast5_metadata_with_enhancer(filename, &metadata_count, metadata_enhancer);
```
The set of arguments to this function includes a metadata enhancer callback, which allows custom processing of the raw metadata read from the Fast5 file.  This design allows users to plug in their own logic for interpreting and augmenting the metadata as needed.  For `ciren` this metadata enhancer looks like:
```c
void metadata_enhancer(hid_t file_id, hid_t signal_dataset_id, fast5_metadata_t *metadata) {
  extract_storage_analysis(signal_dataset_id, metadata);
  extract_pore_level_analysis(file_id, signal_dataset_id, metadata);
  extract_temporal_analysis(file_id, signal_dataset_id, metadata);
}
```
Thus, we tag along other functions to extract specific types of analysis data from the Fast5 file.

`read_fast5_metadata_with_enhancer` is defined in `sequelizer/src/core/fast_io.c`.  And therein, the actual work on a given file is done by one of two functions
```c
if (is_multi_read) {
  metadata = read_multi_read_metadata_with_enhancer(file_id, filename, metadata_count, enhancer);
} else {
  metadata = read_single_read_metadata_with_enhancer(file_id, filename, metadata_count, enhancer);
} 
```
each called depending on whether we have a single-read or a multi-read Fast5 file to deal with.  And inside whichever of these functions is called, we read the various datasets and groups from the HDF5 file, populating our `fast5_metadata_t` structures along the way.  The enhancer callback is invoked to allow custom processing of the metadata as it's being built up.

SO, the overall flow in `ciren_fast5.c` is:
1. Spawn multiple threads, each running `fast5_worker_thread()`, to process subsets of Fast5 files in parallel.
2. Each thread calls `read_fast5_metadata_thread_safe()` for each assigned file, ensuring thread-safe HDF5 access.
3. Inside `read_fast5_metadata_thread_safe()`, the actual reading of file metadata is done by `read_fast5_metadata_with_enhancer()`, which uses an enhancer callback for custom metadata processing.
4. The resulting metadata from all threads is collected and used to compute dataset statistics and an overall analysis summary. 

### In `sequelizer_fast5.c`
```c
// STEP 4
process_files_sequentially(fast5_files, file_count, results, results_count, arguments.verbose, arguments.write_summary);
```
In this function we also process Fast5 files with an enhancer callback, but in a single-threaded manner.  The core reading of each file's metadata is still done via:
```c
fast5_metadata_t *metadata = read_fast5_metadata_with_enhancer(fast5_files[i], &metadata_count, enhancer);
```
just as was done in `read_fast5_metadata_thread_safe()` in the multi-threaded case.

## Notes on HDF5 library

The HDF5 library is NOT thread-safe by default. If multiple threads try to call HDF5 functions simultaneously, you get race conditions and crashes.  `pthread_mutex_t *hdf5_mutex` is a mutual exclusion lock. Before any HDF5 operation, lock the mutex: pthread_mutex_lock(hdf5_mutex).  After HDF5 operations complete, unlock it: `pthread_mutex_unlock(hdf5_mutex)`.  This ensures only ONE thread at a time can execute HDF5 code.