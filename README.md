<div align="center">

<picture>
  <img alt="sequelizer" src="docs/sequelizer.png" width="20%" height="20%">
</picture>

**C toolkit for DNA sequence analysis and nanopore data processing**

</div>

---

## Installation

**Required:**
```bash
# macOS
brew install argp-standalone hdf5
# Ubuntu/Debian
sudo apt-get update && sudo apt-get install -y libargp-dev libhdf5-dev libopenblas-dev pkg-config libcunit1-dev
# Build tools
# CMake 3.23+ and C++17 compiler required
```

**Optional (for testing):**
```bash
# For plotting capabilities
brew install gnuplot feedgnuplot  # macOS
sudo apt-get install gnuplot      # Ubuntu
# For unit tests
brew install cunit                  # macOS
sudo apt-get install libcunit1-dev  # Ubuntu
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

**[📖 Full Documentation](https://emilitronic.github.io/Sequelizer/)** • **[🚀 Getting Started](https://emilitronic.github.io/Sequelizer/getting-started)** • **[Command Reference](https://emilitronic.github.io/Sequelizer/sequelizer_commands)** • **[Fast5 Compatibility](https://emilitronic.github.io/Sequelizer/fast5_compatibility)**
