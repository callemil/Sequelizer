# Sequelizer Commands Reference

Complete usage guide for all Sequelizer subcommands with detailed examples and workflows.

## sequelizer fast5 - Fast5 File Analysis

**Primary command for Fast5 file metadata extraction and analysis.**

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
./sequelizer fast5 debug data.fast5

# Debug problematic files for troubleshooting
./sequelizer fast5 debug problematic_file.fast5

# Debug with verbose output
./sequelizer fast5 debug data.fast5 --verbose
```

### Real Dataset Examples
```bash
# SquiggleFilter project data
./sequelizer fast5 /Users/seb/Documents/GitHub/SquiggleFilter/data/lambda/fast5/ --recursive --verbose

# slow5tools test data
./sequelizer fast5 /Users/seb/Documents/GitHub/slow5tools/test/data --recursive --verbose

# Single file from dataset
./sequelizer fast5 /path/to/dataset/specific_file.fast5 --verbose
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
./sequelizer fast5 debug <input>        # Debug mode with HDF5 structure
./sequelizer fast5 <input> --verbose    # Detailed output
./sequelizer fast5 <input> --recursive  # Process directories recursively

# Combined options
./sequelizer fast5 /path/to/data/ --recursive --verbose
./sequelizer fast5 debug /path/to/data/file.fast5 --verbose
```

## Output Examples

### Standard Info Mode
```
File: data.fast5
Format: multi-read
Reads: 3
File size: 245.2 KB

Read summary:
- Read lengths: 1000-1500 samples
- Sample rates: 4000 Hz
- Total duration: 0.75 seconds
```

### Verbose Mode Output
```
File: data.fast5
Format: multi-read
Reads: 3
File size: 245.2 KB

Read details:
Read 1: read_001
  - Length: 1000 samples
  - Duration: 0.25 seconds
  - Sample rate: 4000 Hz
  - Channel: 126

Read 2: read_002
  - Length: 1200 samples
  - Duration: 0.30 seconds
  - Sample rate: 4000 Hz
  - Channel: 234

Read 3: read_003
  - Length: 1500 samples
  - Duration: 0.375 seconds
  - Sample rate: 4000 Hz
  - Channel: 445
```

### Debug Mode Output
```
File: data.fast5
Format: multi-read (detected via read_* groups)
HDF5 structure:
/
├── read_001/
│   ├── Raw/Signal (dataset: 1000 elements)
│   ├── channel_id/ (group)
│   └── tracking_id/ (group)
├── read_002/
│   ├── Raw/Signal (dataset: 1200 elements)
│   ├── channel_id/ (group)
│   └── tracking_id/ (group)
└── read_003/
    ├── Raw/Signal (dataset: 1500 elements)
    ├── channel_id/ (group)
    └── tracking_id/ (group)

Reads: 3
Total signals: 3700 samples
```

## Workflow Patterns

### Quality Control Workflow
```bash
# Step 1: Quick overview of dataset
./sequelizer fast5 /path/to/sequencing_run/ --recursive

# Step 2: Detailed analysis of interesting files
./sequelizer fast5 /path/to/sequencing_run/subset/ --recursive --verbose

# Step 3: Debug problematic files
./sequelizer fast5 debug /path/to/sequencing_run/problematic_file.fast5
```

### Development Testing Workflow
```bash
# Test basic functionality
./sequelizer fast5 test_data.fast5

# Test error handling
./sequelizer fast5 nonexistent_file.fast5

# Test directory processing
./sequelizer fast5 test_directory/ --recursive

# Test debug capabilities
./sequelizer fast5 debug test_data.fast5
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

# Debug specific file types
find /path/to/data -name "*.fast5" -exec ./sequelizer fast5 debug {} \;
```

## Error Handling Examples

### Common Error Scenarios
```bash
# File not found
$ ./sequelizer fast5 missing.fast5
Error: Cannot access file: missing.fast5

# Invalid Fast5 file
$ ./sequelizer fast5 corrupted.fast5
Warning: Failed to read Fast5 metadata from corrupted.fast5

# Permission denied
$ ./sequelizer fast5 /restricted/data.fast5
Error: Permission denied: /restricted/data.fast5

# Empty directory
$ ./sequelizer fast5 empty_directory/ --recursive
No Fast5 files found in directory: empty_directory/
```

### Debugging Problematic Files
```bash
# Use debug mode for detailed diagnostics
./sequelizer fast5 debug problematic.fast5

# Check if file is actually Fast5 format
file problematic.fast5

# Verify HDF5 structure manually
h5dump -n problematic.fast5

# Test with minimal processing
./sequelizer fast5 problematic.fast5  # (without --verbose)
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
./sequelizer fast5 debug problem_files/
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

## Future Subcommands

### Planned Commands
```bash
# Sequence generation (in development)
./sequelizer seqgen [options]

# Additional Fast5 operations (future)
./sequelizer fast5 convert <input> <output>
./sequelizer fast5 extract <input> --reads read_001,read_002
./sequelizer fast5 validate <input>
```

### Extension Points
- Signal processing capabilities
- Format conversion utilities
- Integration with nanopore analysis pipelines
- Quality control and validation tools