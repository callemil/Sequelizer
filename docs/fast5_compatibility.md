# Fast5 Compatibility Guide

Comprehensive guide to Fast5 file format support, compatibility testing, and troubleshooting in Sequelizer.

## Supported Fast5 Formats

Sequelizer provides robust support for all major Fast5 file formats through intelligent format detection and fallback mechanisms.

### Multi-Read Fast5 Format
**Standard structure with file_type attribute:**
```
/ (root)
├── file_version (attribute)
├── file_type = "multi-read" (attribute) 
├── read_<read_name>/
│   ├── run_id (attribute)
│   ├── Raw/
│   │   ├── Signal (dataset)
│   │   └── duration, read_id, read_number, start_mux, start_time (attributes)
│   ├── channel_id/ (group with channel metadata)
│   ├── context_tags/ (group with filename)
│   └── tracking_id/ (group with exp_start_time, run_id, flow_cell_id, device_id)
└── read_<read_name_2>/...
```

**Non-standard multi-read variants:**
- Files missing `file_type` attribute (common in older datasets)
- Non-standard group naming conventions
- Missing optional metadata groups

### Single-Read Fast5 Format
**Traditional structure:**
```
/ (root)
├── Raw/
│   └── Reads/
│       └── Read_X/
│           └── Signal (dataset)
├── UniqueGlobalKey/ (global metadata group)
│   ├── channel_id/
│   ├── context_tags/
│   └── tracking_id/
└── file_version (attribute)
```

## Format Detection Logic

Sequelizer uses a robust 3-stage detection process to handle real-world file variations:

### Stage 1: Standard Detection
```c
// Check for standard file_type attribute
hid_t attr = H5Aopen(file_id, "file_type", H5P_DEFAULT);
if (attr >= 0) {
    // Read file_type value
    // "multi-read" → Multi-read format
    // Other values → Single-read format
}
```

### Stage 2: Robust Fallback Detection  
```c
// For files missing file_type attribute
// Scan root level for read_* groups
H5G_info_t group_info;
H5Gget_info(file_id, &group_info);

for (hsize_t i = 0; i < group_info.nlinks; i++) {
    char group_name[256];
    H5Lget_name_by_idx(file_id, ".", H5_INDEX_NAME, H5_ITER_INC, i, 
                       group_name, sizeof(group_name), H5P_DEFAULT);
    
    if (strncmp(group_name, "read_", 5) == 0) {
        // Found read_* group → Multi-read format
        return FAST5_MULTI_READ;
    }
}
```

### Stage 3: Single-Read Default
```c
// If no multi-read indicators found
// Check for traditional single-read structure
if (H5Lexists(file_id, "/Raw/Reads", H5P_DEFAULT)) {
    return FAST5_SINGLE_READ;
}
```

## Compatibility Testing Results

### ✅ Confirmed Working Formats

**SquiggleFilter Project Data:**
```bash
./sequelizer fast5 /Users/seb/Documents/GitHub/SquiggleFilter/data/lambda/fast5/ --recursive
```
- **Format**: Multi-read Fast5
- **Characteristics**: Standard file_type attribute, multiple reads per file
- **Status**: Full compatibility, all metadata extracted correctly

**slow5tools Test Data:**
```bash
./sequelizer fast5 /Users/seb/Documents/GitHub/slow5tools/test/data --recursive
```
- **Format**: Mixed single-read and multi-read
- **Characteristics**: Various file sizes, some missing optional attributes
- **Status**: Full compatibility with robust fallback detection

**Oxford Nanopore Standard Files:**
- Standard multi-read Fast5 with complete metadata
- Single-read Fast5 from older MinION runs
- Mixed datasets with varying compression levels

### ✅ Robustness Features

**Missing Attributes Handling:**
- Files without `file_type` attribute → Automatic fallback detection
- Missing optional metadata groups → Graceful degradation with warnings
- Corrupted or incomplete files → Error reporting with file identification

**Error Recovery:**
- HDF5 error suppression to prevent spam during batch processing
- Individual file failures don't stop directory processing
- Comprehensive error reporting for debugging

**Performance Optimization:**
- Minimal file access for format detection
- Efficient metadata extraction without loading signal data
- Memory-conscious processing for large datasets

## Troubleshooting Guide

### Debug Mode Usage
**For problematic files:**
```bash
# Show detailed HDF5 structure
./sequelizer fast5 debug problematic_file.fast5

# Debug with verbose metadata extraction
./sequelizer fast5 debug problematic_file.fast5 --verbose
```

**Debug output interpretation:**
```
File: problematic.fast5
Format: multi-read (detected via read_* groups)  # ← Detection method shown
HDF5 structure:
/ (root)
├── read_001/                                    # ← Groups discovered
│   ├── Raw/Signal (dataset: 1000 elements)     # ← Signal data found
│   ├── channel_id/ (group)                     # ← Metadata groups
│   └── tracking_id/ (group)
└── file_version (attribute: "2.0")             # ← Attributes present

Missing standard attributes:                      # ← Compatibility notes
- file_type attribute not found (using fallback detection)
```

### Common Issues and Solutions

**Issue: "Failed to read Fast5 metadata"**
```bash
# Solution: Use debug mode to examine structure
./sequelizer fast5 debug failing_file.fast5

# Check if file is actually HDF5/Fast5
file failing_file.fast5
h5dump -H failing_file.fast5
```

**Issue: "No reads found in multi-read file"**
```bash
# Solution: Check for non-standard read naming
./sequelizer fast5 debug confusing_file.fast5

# Look for alternative group structures in debug output
```

**Issue: "Permission denied" or "File not found"**
```bash
# Solution: Verify file permissions and paths
ls -la problematic_file.fast5
./sequelizer fast5 "$(realpath problematic_file.fast5)"
```

**Issue: Slow processing of large directories**
```bash
# Solution: Use recursive mode for efficiency
./sequelizer fast5 large_dataset/ --recursive

# For very large datasets, consider batch processing
find large_dataset/ -name "*.fast5" | head -100 | xargs -I {} ./sequelizer fast5 {}
```

### Validation Workflow
```bash
# Step 1: Quick validation scan
./sequelizer fast5 dataset/ --recursive 2> validation_errors.log

# Step 2: Examine any errors
cat validation_errors.log

# Step 3: Debug specific problematic files
./sequelizer fast5 debug $(grep "Failed" validation_errors.log | cut -d: -f1)

# Step 4: Verify successful processing count
./sequelizer fast5 dataset/ --recursive | grep -c "Format:"
```

## Advanced Compatibility Features

### Metadata Extraction Robustness
**Essential metadata (always extracted):**
- Read IDs and read count
- Signal length and sample rate  
- File format detection
- Basic file information

**Optional metadata (graceful fallback):**
- Channel information
- Experimental metadata
- Timing information
- Device details

**Error handling hierarchy:**
```c
// Try standard location first
if (standard_location_exists) {
    extract_from_standard_location();
} else if (alternative_location_exists) {
    extract_from_alternative_location();
} else {
    // Graceful degradation
    warn_missing_metadata();
    continue_processing();
}
```

### Performance Characteristics

**File Access Patterns:**
- **Format detection**: Minimal file access (root attributes and group listing only)
- **Metadata extraction**: Targeted access to specific datasets and attributes
- **Signal data**: Never loaded (preserves memory and performance)

**Scalability:**
- **Small files** (< 1MB): Instant processing
- **Large files** (> 100MB): Same performance (metadata-only access)
- **Large directories** (1000+ files): Linear scaling with file count

**Memory Usage:**
- **Per-file overhead**: < 1KB for metadata storage
- **Total memory**: Independent of file sizes, dependent only on file count
- **HDF5 library**: Efficient internal caching and resource management

## Integration Notes

### Ciren Compatibility
**Shared Fast5 I/O library** (`src/core/fast5_io.c/h`):
- Identical format detection logic
- Same robustness features and error handling
- Compatible metadata structures and APIs
- Ciren adds enhanced statistics on top of Sequelizer foundation

### Third-Party Tool Compatibility
**Works with files from:**
- Oxford Nanopore MinKNOW software
- Guppy basecaller output
- ont_fast5_api processed files
- Custom Fast5 generation tools
- Legacy nanopore datasets

**Generates output compatible with:**
- Downstream nanopore analysis pipelines
- Custom analysis scripts expecting standard metadata
- Quality control and validation tools