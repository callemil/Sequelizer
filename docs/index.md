---
layout: default
title: Sequelizer Documentation
description: Open-source C toolkit for DNA sequence analysis and nanopore data processing
---

# Sequelizer Documentation

<div align="center">

<picture>
  <img alt="sequelizer" src="sequelizer.png" width="20%" height="20%">
</picture>

<p><strong>Open-source C toolkit for DNA sequence analysis and nanopore data processing</strong></p>

</div>

---

## Quick Start

```bash
# Build Sequelizer
mkdir build && cd build
cmake .. && cmake --build .
# Analyze Fast5 files
./sequelizer fast5 data.fast5
./sequelizer fast5 /path/to/dataset/ --recursive --verbose
# Extract raw signals
./sequelizer convert data.fast5 --to raw
./sequelizer convert /path/to/dataset/ --to raw --recursive --all
```

## What is Sequelizer?

Sequelizer is a C-based toolkit designed for nanopore DNA/RNA sequencing analysis. It provides robust, efficient tools for:

- **Fast5 file analysis** - Comprehensive metadata extraction and format validation
- **Signal conversion** - Raw signal extraction for downstream analysis  
- **Cross-platform support** - Clean, portable C implementation
- **Integration-ready** - Foundation for advanced tools like Ciren

### Key Features

- **Robust Fast5 support** - Handles standard and non-standard file formats with intelligent fallback detection
- **Minimal dependencies** - Only requires HDF5 and CMake for maximum portability
- **High performance** - Efficient metadata-only processing, memory-conscious design
- **Clean architecture** - Modular subcommand design for easy extension
- **Battle-tested** - Validated against real-world nanopore datasets

## Core Commands

### sequelizer fast5 - Fast5 Analysis
Comprehensive Fast5 file metadata extraction and validation.

```bash
# Single file analysis
./sequelizer fast5 data.fast5
# Dataset analysis with full details
./sequelizer fast5 /path/to/dataset/ --recursive --verbose
# Debug problematic files
./sequelizer fast5 problematic.fast5 --debug
```

**[→ Complete Fast5 command guide](sequelizer_commands.md#sequelizer-fast5---fast5-file-analysis)**

### sequelizer convert - Signal Extraction
Extract raw signals from Fast5 files for downstream analysis.

```bash
# Convert single file to raw signals
./sequelizer convert data.fast5 --to raw
# Batch convert with all reads
./sequelizer convert /path/to/dataset/ --to raw --recursive --all --output signals/
```

**[→ Complete convert command guide](sequelizer_commands.md#sequelizer-convert---fast5-format-conversion)**

## Documentation

### User Guides
- **[Commands Reference](sequelizer_commands.md)** - Complete usage guide for all commands
- **[Fast5 Compatibility](fast5_compatibility.md)** - Format support and troubleshooting

### Technical Documentation
- **[Build Instructions](#build-system)** - Compilation and dependencies
- **[Architecture Overview](#architecture)** - Core design and extension points
- **[Integration Guide](#integration)** - Using Sequelizer in pipelines

## Build System

### Requirements
- **CMake 3.23+** with C17 compiler support
- **HDF5 library** for Fast5 file format support

### macOS Setup
```bash
# Install dependencies
brew install hdf5
# Build Sequelizer
mkdir build && cd build
cmake .. && cmake --build .
# Executable: build/sequelizer
```

### Linux Setup
```bash
# Ubuntu/Debian
sudo apt-get install libhdf5-dev cmake
# CentOS/RHEL  
sudo yum install hdf5-devel cmake
# Build
mkdir build && cd build
cmake .. && cmake --build .
```

### Build Targets
- `sequelizer` - Main executable with all subcommands
- `sequelizer_static` - Static library for integration

## Architecture

### Clean Subcommand Design
```
src/
├── core/
│   ├── fast5_io.c/h          # Shared Fast5 I/O (used by Ciren)
│   ├── fast5_utils.c/h       # File utilities and metadata
│   └── util.c/h              # Common utilities
├── sequelizer.c              # Minimal main() with routing
├── sequelizer_subcommands.c/h # Command routing and help
├── sequelizer_fast5.c/h      # Fast5 analysis implementation
└── sequelizer_convert.c/h    # Signal conversion implementation
```

### Key APIs
```c
// Fast5 file discovery
char **find_fast5_files(const char *path, bool recursive, int *count);
// Metadata extraction
fast5_metadata_t *read_fast5_metadata(const char *filename, int *count);
// Format detection (automatic and robust)
fast5_format_t detect_fast5_format(hid_t file_id);
```

### Extension Points
Adding new subcommands follows a simple pattern:
1. Create `sequelizer_<command>.c/h` files
2. Add enum entry and detection logic
3. Update CMakeLists.txt
4. Follow established error handling and help patterns

## Integration

### Pipeline Integration
```bash
# Generate file lists for processing
./sequelizer fast5 dataset/ --recursive > file_list.txt
# Extract signals for analysis
./sequelizer convert dataset/ --to raw --recursive --all -o signals/
# Validate files before processing
./sequelizer fast5 dataset/ --recursive 2> validation.log
```

### Ciren Integration
Sequelizer serves as the open-source foundation for Ciren:

```bash
# Use Sequelizer for initial analysis
./sequelizer fast5 dataset/ --recursive --verbose
# Use Ciren for advanced features
../ciren/build/ciren fast5 dataset/ --format json --enhanced-stats
```

**Shared components:**
- `src/core/fast5_io.c/h` - Identical Fast5 I/O implementation
- Build patterns and architectural principles
- Compatible APIs and data structures

## Tested Datasets

Sequelizer has been validated against real-world nanopore datasets:

```bash
# SquiggleFilter project data
./sequelizer fast5 /path/to/SquiggleFilter/data/lambda/fast5/ --recursive
# slow5tools test data  
./sequelizer fast5 /path/to/slow5tools/test/data --recursive
# Various Oxford Nanopore formats
# - Standard multi-read Fast5
# - Legacy single-read Fast5  
# - Non-standard variants missing file_type attributes
```

## Performance

### Characteristics
- **Metadata-only processing** - Signal data never loaded into memory
- **Efficient file access** - Minimal HDF5 operations per file
- **Scalable design** - Linear performance with file count, not file size
- **Memory conscious** - < 1KB overhead per file processed

### Benchmarks
- **Small files** (< 1MB): Instant processing
- **Large files** (> 100MB): Same performance (metadata-only)
- **Large directories** (1000+ files): Efficient batch processing

## Support and Development

### Getting Help
- **Documentation**: Start with this index and the [commands reference](sequelizer_commands.md)
- **Troubleshooting**: See [Fast5 compatibility guide](fast5_compatibility.md)
- **Issues**: Report problems with specific file examples and debug output

### Contributing
- **Code style**: 2-space indentation, descriptive names
- **Testing**: Validate against real nanopore datasets
- **Documentation**: Update relevant guides for new features

### License
Sequelizer is open-source software. See [LICENSE](../LICENSE) for details.

---

## Quick Reference

### Most Common Operations
```bash
# Analyze a dataset
./sequelizer fast5 /path/to/data/ --recursive --verbose
# Extract all signals from multi-read files  
./sequelizer convert /path/to/data/ --to raw --all --recursive -o signals/
# Debug a problematic file
./sequelizer fast5 problem_file.fast5 --debug
# Get help for any command
./sequelizer <command> --help
```

### Next Steps
1. **[Try the commands](sequelizer_commands.md)** - Start with Fast5 analysis
2. **[Understand compatibility](fast5_compatibility.md)** - Learn about format support
3. **[Integrate with pipelines](#integration)** - Use in your analysis workflow
4. **[Explore Ciren](../CLAUDE.md)** - Advanced features and performance enhancements

*For the complete feature set and enhanced performance, consider [Ciren](../CLAUDE.md) which builds on Sequelizer's foundation.*