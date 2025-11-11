# Sequelizer Linux Port Checklist

Based on successful Ciren Linux port (commit with Ubuntu compatibility changes).

## CMakeLists.txt Changes Required

### 1. Platform-Specific Dependency Detection

**Current** (macOS-only):
```cmake
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(HDF5_ROOT "/opt/homebrew/opt/hdf5")
else()
    set(HDF5_ROOT "/usr/local/opt/hdf5")
endif()
```

**Target** (cross-platform):
```cmake
if(APPLE)
    # macOS Homebrew paths
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(HDF5_ROOT "/opt/homebrew/opt/hdf5")
        set(ARGP_ROOT "/opt/homebrew/opt/argp-standalone")
    else()
        set(HDF5_ROOT "/usr/local/opt/hdf5")
        set(ARGP_ROOT "/usr/local/opt/argp-standalone")
    endif()

    if (HDF5_ROOT)
        include_directories("${HDF5_ROOT}/include")
        link_directories("${HDF5_ROOT}/lib")
    endif()

    if (ARGP_ROOT)
        include_directories("${ARGP_ROOT}/include")
        link_directories("${ARGP_ROOT}/lib")
    endif()
else()
    # Linux: Use pkg-config for library discovery
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(HDF5 REQUIRED hdf5)

    include_directories(${HDF5_INCLUDE_DIRS})
    link_directories(${HDF5_LIBRARY_DIRS})
endif()
```

### 2. Linker Configuration

**Current**:
```cmake
target_link_libraries(sequelizer sequelizer_static m hdf5 argp)
```

**Target**:
```cmake
if(APPLE)
    target_link_libraries(sequelizer sequelizer_static m hdf5 argp)
else()
    target_link_libraries(sequelizer sequelizer_static m ${HDF5_LIBRARIES})
endif()
```

Note: Linux includes argp in glibc by default (no separate -largp needed).

## Source Code Changes Required

### argp Return Type

**Files to check**:
- Any file using `static errno_t parse_arg(...)` function

**Change**:
```c
// OLD (macOS-specific)
static errno_t parse_arg(int key, char *arg, struct argp_state *state) {
    // ...
}

// NEW (cross-platform)
static int parse_arg(int key, char *arg, struct argp_state *state) {
    // ...
}
```

**Why**: macOS argp uses `errno_t`, Linux argp uses `int`. Using `int` works on both.

**Likely affected files** (based on Ciren patterns):
- `src/sequelizer_fast5.c` (if it uses argp)
- `src/sequelizer_seqgen.c` (if it uses argp)
- `src/sequelizer_convert.c` (if it uses argp)
- `src/sequelizer_plot.c` (if it uses argp)

### Other Platform-Specific Code

**Check for**:
1. Platform-specific includes (should be minimal in Sequelizer)
2. macOS-specific system calls
3. Hardcoded paths

**Sequelizer should NOT need**:
- BLAS header changes (no BLAS dependency)
- Accelerate framework includes (not used)
- OpenBLAS linking (not used)

## Testing on Linux

```bash
# Install dependencies
sudo apt-get update && sudo apt-get install -y \
  libargp-dev libhdf5-dev pkg-config cmake build-essential

# Build
cd sequelizer
mkdir build && cd build
cmake ..
cmake --build .

# Test
./sequelizer --help
./sequelizer fast5 <test-file.fast5>
```

## Files to Modify

1. ✅ `CMakeLists.txt` - Platform detection and linking
2. ❓ `src/sequelizer_fast5.c` - Check for argp usage
3. ❓ `src/sequelizer_seqgen.c` - Check for argp usage
4. ❓ `src/sequelizer_convert.c` - Check for argp usage
5. ❓ `src/sequelizer_plot.c` - Check for argp usage
6. ✅ `CLAUDE.md` - Already updated with Linux notes
7. ❓ `README.md` (if exists) - Add Linux dependencies

## Reference: Ciren Changes

See Ciren commit with message about Ubuntu Linux compatibility:
- CMakeLists.txt: Lines 12-36 (platform detection), 64-74 (flags), 107-111 (linking)
- ciren_bcall.c: Line 150 (`errno_t` → `int`)
- ciren_seqgen.c: Line 117 (`errno_t` → `int`)
- ciren_matrix.c: Lines 10-13 (BLAS headers - NOT needed for Sequelizer)
- layers.c: Lines 11-14 (BLAS headers - NOT needed for Sequelizer)
- ciren_fast5.c: Lines 228-231 (edge case fix)
- README.md: Updated Ubuntu dependencies

## Key Difference: No BLAS Dependency

**Ciren** requires:
- macOS: Accelerate framework
- Linux: OpenBLAS

**Sequelizer** requires:
- macOS: HDF5, argp-standalone
- Linux: HDF5, libargp-dev (if not in glibc)

This simplifies Sequelizer's Linux port significantly!
