<div align="center">

<picture>
  <img alt="sequelizer" src="docs/sequelizer.png" width="20%" height="20%">
</picture>

**C toolkit for DNA sequence analysis and nanopore data processing**

</div>

---

## Installation

### Prerequisites
```bash
# macOS
brew install hdf5

# Linux (Ubuntu/Debian)
sudo apt-get install libhdf5-dev
```

### Build
```bash
mkdir build && cd build
cmake .. && cmake --build .
```

### Quick Start
```bash
# Analyze Fast5 files
./sequelizer fast5 data.fast5 --recursive --verbose

# Convert to text format
./sequelizer convert data.fast5 --to raw

# Plot signals
./sequelizer plot signals.txt
```

---

**[📖 Full Documentation](docs/index.md)** • **[Command Reference](docs/sequelizer_commands.md)** • **[Fast5 Compatibility](docs/fast5_compatibility.md)**
