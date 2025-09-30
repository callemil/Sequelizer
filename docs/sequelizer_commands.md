---
layout: default
title: Commands Reference
description: Complete usage guide for all Sequelizer subcommands with detailed examples and workflows
---

# Sequelizer Commands Reference
Complete usage guide for all Sequelizer subcommands with detailed examples and workflows.

## Table of Contents

- [sequelizer fast5 - Fast5 File Analysis](#sequelizer-fast5---fast5-file-analysis)
- [sequelizer convert - Fast5 Format Conversion](#sequelizer-convert---fast5-format-conversion)
- [sequelizer plot - Visualizing Measurements](#sequelizer-plot---visualizing-measurements)
- [Command-Line Options](#command-line-options)
- [Output Examples](#output-examples)
- [Workflow Patterns](#workflow-patterns)
- [Performance Guidelines](#performance-guidelines)
- [Integration with Other Tools](#integration-with-other-tools)
- [Future Subcommands](#future-subcommands)

---

## sequelizer fast5 - Fast5 File Analysis
**Primary command for Fast5 file metadata extraction and analysis.**

Use this to skim across your fast5 directories and get some basic stats on the fast5 contents.
### Basic Usage
```bash
# Single file analysis (default: info mode)
./sequelizer fast5 data.fast5
# Display help for fast5 subcommand
./sequelizer fast5 --help
# Verbose output with detailed read information
./sequelizer fast5 data.fast5 --verbose
```
### Directory Operations
```bash
# Analyze directory (non-recursive)
./sequelizer fast5 /path/to/fast5_files/
# Recursive directory analysis (RECOMMENDED for datasets)
./sequelizer fast5 /path/to/fast5_files/ --recursive
# Recursive with verbose output
./sequelizer fast5 /path/to/fast5_files/ --recursive --verbose
```
### Debug Mode
```bash
# Debug single file - shows detailed HDF5 structure
./sequelizer fast5 data.fast5 --debug
# Debug problematic files for troubleshooting
./sequelizer fast5 problematic_file.fast5 --debug
# Debug with verbose output
./sequelizer fast5 data.fast5 --verbose --debug
```

## Command-Line Options
### Global Options
```bash
# Display main help
./sequelizer
./sequelizer --help
./sequelizer help
# Version information
./sequelizer --version
```
### Fast5 Subcommand Options
```bash
# Basic operations
./sequelizer fast5 <input>              # Default info mode
./sequelizer fast5 <input> --debug      # Debug mode with HDF5 structure
./sequelizer fast5 <input> --verbose    # Detailed output
./sequelizer fast5 <input> --recursive  # Process directories recursively
# Combined options
./sequelizer fast5 /path/to/data/ --recursive --verbose
./sequelizer fast5 /path/to/data/file.fast5 --verbose --debug
```

## Output Examples
### Standard Info Mode on One File
```
[████████████████████████████████████████] 100% (1/1)

Fast5 File: /some/set/of/directories/some_file.fast5
=====================================
File size: 4.71 MB
Format: Multi-read
Reads: 250
Sample rate: 4000 Hz
Signal statistics:
  Total samples: 2525348
  Average length: 10101 samples
  Range: 2152 - 39504 samples
  Total duration: 631.3 seconds
  Average duration: 2.5 seconds

Showing first 3 reads (use --verbose for all):
  Read 1: 01d9d672-8f2f-4b97-a81f-c9ee8c898ade (8518 samples)
  Read 2: 0203a671-1dd3-43f6-85b1-7d6d5b3b13e2 (3094 samples)
  Read 3: 073e5b10-48d5-4b89-8268-708c601365bc (6169 samples)
  ... and 247 more reads

Sequelizer Fast5 Dataset Analysis Summary
=========================================
Files processed: 1/1 successful
Total file size: 4.7 MB
Total reads: 250
Signal statistics:
  Total samples: 2525348
  Average length: 10101 samples
  Range: 2152 - 39504 samples
  Average bits per sample: 14.93
  Total duration: 10.5 minutes
  Avg duration: 2.5 seconds
Processing time: 0.04 seconds
```
### Standard Info Mode on a Directory
```
[████████████████████████████████████████] 100% (86/86)

Sequelizer Fast5 Dataset Analysis Summary
=========================================
Files processed: 86/86 successful
Total file size: 744.8 MB
Total reads: 21500
Signal statistics:
  Total samples: 491461379
  Average length: 22859 samples
  Range: 1913 - 355054 samples
  Average bits per sample: 12.12
  Total duration: 2047.8 minutes
  Avg duration: 5.7 seconds
Processing time: 2.47 seconds
```
### Looking Recursively Through Directories (some files fail)
```
[████████████████████████░░░░░░░░░░░░░░░░] 62% (48/77)sequelizer: Failed to open Fast5 file: /a/directory/a_file.fast5
[████████████████████████████░░░░░░░░░░░░] 71% (55/77)sequelizer: Failed to open Fast5 file: /another/directory/some_other_file.fast5
[████████████████████████████████████████] 100% (77/77)

Sequelizer Fast5 Dataset Analysis Summary
=========================================
Files processed: 72/77 successful (5 failed)
Total file size: 88.1 MB
Total reads: 1105
Signal statistics:
  Total samples: 56749637
  Average length: 51357 samples
  Range: 4479 - 376770 samples
  Average bits per sample: 12.42
  Total duration: 321.4 minutes
  Avg duration: 17.5 seconds
Processing time: 0.03 seconds
```
### Debug Mode Output
```
# todo
```

## Workflow Patterns
### Quality Control Workflow
```bash
# Step 1: Quick overview of dataset
./sequelizer fast5 /path/to/sequencing_run/ --recursive
# Step 2: Detailed analysis of interesting files
./sequelizer fast5 /path/to/sequencing_run/subset/ --recursive --verbose
```
### Batch Processing Workflow
```bash
# Process multiple directories
for dir in run_*/; do
    echo "Processing $dir"
    ./sequelizer fast5 "$dir" --recursive > "${dir%/}_analysis.txt"
done
# Generate summary reports
./sequelizer fast5 all_runs/ --recursive --verbose > comprehensive_report.txt
```

## Performance Guidelines
### File Processing Performance
- **Single files**: Instant metadata extraction
- **Small directories** (< 100 files): Process individually with details
- **Large directories** (100+ files): Use recursive mode for efficiency
- **Very large datasets** (1000+ files): Consider batch processing scripts

### Memory Usage
- **Minimal footprint**: Only metadata loaded, not signal data
- **Scalable**: Memory usage independent of file sizes
- **Efficient**: HDF5 library handles file access optimization

### Recommended Usage Patterns
```bash
# For exploration: start with non-verbose
./sequelizer fast5 unknown_dataset/ --recursive
# For analysis: add verbose for details
./sequelizer fast5 known_dataset/ --recursive --verbose
# For troubleshooting: use debug mode
./sequelizer fast5 problem_files/ --debug
```

## Integration with Other Tools
### Pipeline Integration
```bash
# Generate file lists for downstream processing
./sequelizer fast5 dataset/ --recursive | grep "multi-read" > multi_read_files.txt
# Extract metadata for analysis
./sequelizer fast5 dataset/ --recursive --verbose > dataset_metadata.txt
# Validate file integrity before processing
./sequelizer fast5 dataset/ --recursive 2> validation_errors.log
```

### Ciren Integration
```bash
# Use Sequelizer for initial analysis
./sequelizer fast5 dataset/ --recursive --verbose
# Then use Ciren for advanced analysis
../ciren/build/ciren fast5 dataset/ --recursive --format json
```

## sequelizer convert - Fast5 Format Conversion
**Convert Fast5 files to different output formats for downstream analysis.**

### Basic Usage
```bash
# Convert single file to raw (default: 3 reads for multi-read files)
# NOTE: for single-read files this produces files name: read_ch<channel num>_rd<read num>.txt
# but for multi-read files this produces files named: <original fast5 files name>_read_ch<channe _num>_rd<read num>.txt
./sequelizer convert data.fast5 --to raw
# Display help for convert subcommand
./sequelizer convert --help
# Convert with verbose output
./sequelizer convert data.fast5 --to raw --verbose
```
### Output Options
```bash
# Convert to raw signals with specific output name (note: single-read files interpret output as a file name, not a directory, while multi-read reafiles interpret the output as a directory only)
./sequelizer convert data.fast5 --to raw -o output.txt
# NOTE: currently single read files interpret outputs as file names, so putting "/" in the name (as below) will result in error
./sequelizer convert data.fast5 --to raw --output signals/
# Extract all reads (for multi-read files)
./sequelizer convert multi_read.fast5 --to raw --all
# Convert all reads with output directory
# NOTE: for multi-read files when you do specify an output, the file names will be read_ch<channel num>_rd<read num>.txt
./sequelizer convert multi_read.fast5 --to raw --all -o signals
```
### Directory Operations
```bash
# Convert directory (non-recursive)
./sequelizer convert /path/to/fast5_files/ --to raw
# Recursive directory conversion (RECOMMENDED for datasets)
./sequelizer convert /path/to/fast5_files/ --to raw --recursive
# Recursive with verbose output and output directory
./sequelizer convert /path/to/fast5_files/ --to raw --recursive --verbose -o converted/
```
### Real Dataset Examples
```bash
# SquiggleFilter project data - extract all raw signals
./sequelizer convert /Users/seb/Documents/GitHub/SquiggleFilter/data/lambda/fast5/ --to raw --recursive --all -o lambda_signals/
# Single multi-read file with all reads
./sequelizer convert FAL11227_multi.fast5 --to raw --all -o signals/
# Single-read file conversion
./sequelizer convert read_ch271_file66.fast5 --to raw -o read271_signals.txt
```

### Command-Line Options
#### Convert Subcommand Options
```bash
# Format selection
./sequelizer convert <input> --to raw                   # Extract raw signal data (ONLY supported format)
# Output specification
./sequelizer convert <input> --to raw -o <output>       # Short form
./sequelizer convert <input> --to raw --output <output> # Long form
# Read selection (multi-read files only)
./sequelizer convert <input> --to raw --all             # Extract all reads (default: first 3)
# Directory processing
./sequelizer convert <input> --to raw --recursive       # Process directories recursively
# Verbosity
./sequelizer convert <input> --to raw --verbose         # Detailed output during conversion
```

### Output File Naming
#### Single-read Files
```bash
# Without -o flag: automatic naming using channel and read numbers
./sequelizer convert read_ch271_file66.fast5 --to raw
# Output: read_ch271_rd66.txt
# With -o flag: use specified filename
./sequelizer convert read_ch271_file66.fast5 --to raw -o my_signal.txt
# Output: my_signal.txt
```

#### Multi-read Files
```bash
# Without -o flag: basename with channel/read suffixes
./sequelizer convert FAL11227_multi.fast5 --to raw
# Output: FAL11227_multi_read_ch126_rd1.txt, FAL11227_multi_read_ch234_rd2.txt, etc.
# With -o directory: clean naming in specified directory
./sequelizer convert FAL11227_multi.fast5 --to raw -o signals/
# Output: signals/read_ch126_rd1.txt, signals/read_ch234_rd2.txt, etc.
```

### Output Examples
#### Single-Read Conversion with Verbose
```
$ ./sequelizer convert data.fast5 --to raw --verbose
Found 1 files to convert
Output format: raw

Converting 1 files to raw format...
Processing file: data.fast5
  Wrote 29150 samples to: read_ch228_rd118.txt
```

#### Multi-Read Verbose Mode Output
```
$ ./sequelizer convert multi_read.fast5 --to raw --verbose
Found 1 files to convert
Output format: raw

Converting 1 files to raw format...
Processing file: multi_read.fast5
  Multi-read file: processing first 3 of 250 reads (use --all for all)
  Wrote 8518 samples to: multi_read_read_ch44_rd26.txt
  Wrote 3094 samples to: multi_read_read_ch304_rd43.txt
  Wrote 6169 samples to: multi_read_read_ch477_rd63.txt
```

#### Directory Processing Output
```
$ ./sequelizer convert dataset/ --to raw --recursive --verbose
Found 77 files to convert
Output format: raw

Converting 77 files to raw format...
[░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░] 0% (0/77) converting filesProcessing file: dataset/raw/f2s/end_reason_fast5/end_reason_datatype_uint8_t.fast5
  Wrote 23469 samples to: end_reason_datatype_uint8_t_read_ch11_rd42.txt
[░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░] 1% (1/77) converting filesProcessing file: dataset/raw/f2s/end_reason_fast5/end_reason_differnt_key_order.fast5
  Wrote 5298 samples to: end_reason_differnt_key_order_read_ch341_rd3586.txt
  Wrote 4479 samples to: end_reason_differnt_key_order_read_ch281_rd11189.txt
[█░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░] 2% (2/77) converting filesProcessing file: dataset/raw/f2s/end_reason_fast5/end_reason_int32.fast5
  Wrote 5298 samples to: end_reason_int32_read_ch341_rd3586.txt
  Wrote 4479 samples to: end_reason_int32_read_ch281_rd11189.txt
[█░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░] 3% (3/77) converting filesProcessing file: dataset/raw/f2s/end_reason_fast5/end_reason_new_key.fast5
  Wrote 5298 samples to: end_reason_new_key_read_ch341_rd3586.txt
[██░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░] 5% (4/77) converting filesProcessing file: dataset/raw/f2s/end_reason_fast5/end_reason1.fast5
  Wrote 14567 samples to: end_reason1_read_ch1960_rd13875.txt
  Wrote 50934 samples to: end_reason1_read_ch1061_rd12263.txt
...
```

### Workflow Patterns
#### Signal Analysis Workflow
```bash
# Step 1: Extract raw signals from experimental data
./sequelizer convert experimental_run/ --to raw --recursive --all -o raw_signals/
# Step 2: Process specific files of interest (note: only multi-read files can be output to a directory)
./sequelizer convert interesting_file.fast5 --to raw --verbose -o analysis/
# Step 3: Batch convert multiple datasets
for dataset in run_*/; do
    ./sequelizer convert "$dataset" --to raw --recursive -o "signals_${dataset%/}/"
done
```

#### Development Testing Workflow  
```bash
# Test single file conversion
./sequelizer convert test_data.fast5 --to raw
# Test multi-read extraction
./sequelizer convert multi_test.fast5 --to raw --all --verbose
# Test directory processing
./sequelizer convert test_directory/ --to raw --recursive
```

#### Pipeline Integration Workflow
```bash
# Extract signals for downstream basecalling
./sequelizer convert raw_data/ --to raw --recursive --all -o signals/
# Generate signal files for analysis tools
find dataset/ -name "*.fast5" -exec ./sequelizer convert {} --to raw -o processed/ \;
# Batch process with progress tracking
./sequelizer convert large_dataset/ --to raw --recursive --verbose > conversion.log
```

### Error Handling Examples
#### Common Error Scenarios
```bash
# File not found
$ ./sequelizer convert missing.fast5 --to raw
Error: Cannot access file: missing.fast5
# Invalid Fast5 file
$ ./sequelizer convert corrupted.fast5 --to raw
Warning: Cannot read metadata from file: corrupted.fast5
# Permission denied
$ ./sequelizer convert /restricted/data.fast5 --to raw
Error: Permission denied: /restricted/data.fast5
# Output directory creation failure
$ ./sequelizer convert data.fast5 --to raw -o /readonly/output/
Warning: Cannot create directory: /readonly/output/
```

### Signal Format Details
#### Raw Signal Output Format
- **File format**: Plain text, one signal value per line
- **Precision**: 6 decimal places (e.g., 123.456789)
- **Units**: Raw current measurements in picoamperes (pA)
- **Encoding**: ASCII text, newline-separated values

#### Example Signal File Content
```
# Channel: 228
# Offset: 22.000000
# Range: 1456.560000
# Digitisation: 8192.000000
# Conversion: signal_pA = (raw_signal + offset) * range / digitisation
# Sample Rate: 4000.0
# Read ID: e47468bf-12e3-4208-a866-babdd780e9c0
#
356
260
258
242
...
```

### Performance Guidelines
#### Conversion Performance
- **Single files**: Instant extraction for small files (< 10MB)
- **Multi-read files**: ~100-500ms per file depending on read count
- **Large directories**: Process in batches for datasets > 1000 files
- **Memory usage**: Minimal footprint, processes one file at a time

#### Recommended Usage Patterns
```bash
# For small datasets: process all at once
./sequelizer convert small_dataset/ --to raw --recursive --all
# For large datasets: use verbose mode for progress tracking
./sequelizer convert large_dataset/ --to raw --recursive --verbose
# For development: test with limited reads first
./sequelizer convert test_files/ --to raw --recursive  # (default: first 3 reads)
```

### Integration with Other Tools
#### Pipeline Integration
```bash
# Extract signals for custom analysis scripts
./sequelizer convert dataset/ --to raw --recursive -o signals/
python analyze_signals.py signals/
# Generate training data for machine learning
./sequelizer convert training_set/ --to raw --all --recursive -o ml_data/
# Prepare signals for visualization
./sequelizer convert interesting_reads/ --to raw -o plots/
gnuplot -e "plot 'plots/read_ch271_rd66.txt' with lines"
```
#### Ciren Integration
```bash
# Use Sequelizer for signal extraction
./sequelizer convert dataset/ --to raw --recursive -o signals/
# Then use Ciren for advanced analysis
../ciren/build/ciren bcall signals/ --model-path models/
```


## sequelizer plot - Visualizing Measurements
**Taking a look at signals.**

## Future Subcommands
### Planned Commands
```bash
# Sequence generation (in development)
./sequelizer seqgen [options]
# Additional Fast5 operations (future)
./sequelizer fast5 extract <input> --reads read_001,read_002
./sequelizer fast5 validate <input>
```
### Extension Points
- Signal processing capabilities
- Format conversion utilities
- Integration with nanopore analysis pipelines
- Quality control and validation tools