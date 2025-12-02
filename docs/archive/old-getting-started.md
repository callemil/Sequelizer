---
layout: default
title: Getting Started
description: Installation and first steps with Sequelizer
---

# Getting Started with Sequelizer

This guide will help you install Sequelizer and run your first commands.

## Installation and Build

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

## Quick Start

Once you have Sequelizer built, here are the essential commands to get started:

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

## First Steps

After building Sequelizer, try these commands:

### 1. Analyze a Fast5 File

```bash
# Single file analysis
./sequelizer fast5 data.fast5
# Dataset analysis with full details
./sequelizer fast5 /path/to/dataset/ --recursive --verbose
# Debug problematic files
./sequelizer fast5 problematic.fast5 --debug
```

### 2. Extract Raw Signals

```bash
# Convert single file to raw signals
./sequelizer convert data.fast5 --to raw
# Batch convert with all reads
./sequelizer convert /path/to/dataset/ --to raw --recursive --all --output signals/
```

### 3. Get Help

```bash
# General help
./sequelizer --help
# Command-specific help
./sequelizer fast5 --help
./sequelizer convert --help
```

## Next Steps

- **[Commands Reference](sequelizer_commands.md)** - Complete usage guide for all commands
- **[Fast5 Compatibility](fast5_compatibility.md)** - Format support and troubleshooting
- **[Back to Documentation Home](index.md)** - Overview and architecture details
