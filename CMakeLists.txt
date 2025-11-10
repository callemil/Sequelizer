cmake_minimum_required(VERSION 3.23)
project(Sequelizer VERSION 0.1.0)

set(CMAKE_C_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

# HDF5 dependency for Fast5 file I/O
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(HDF5_ROOT "/opt/homebrew/opt/hdf5")
else()
    set(HDF5_ROOT "/usr/local/opt/hdf5")
endif()
if (HDF5_ROOT)
    include_directories("${HDF5_ROOT}/include")
    link_directories("${HDF5_ROOT}/lib")
    set(CMAKE_REQUIRED_INCLUDES ${HDF5_ROOT}/include)
endif()

# argp-standalone dependency for command line parsing
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(ARGP_ROOT "/opt/homebrew/opt/argp-standalone")
else()
    set(ARGP_ROOT "/usr/local/opt/argp-standalone")
endif()
if (ARGP_ROOT)
    include_directories("${ARGP_ROOT}/include")
    link_directories("${ARGP_ROOT}/lib")
    set(CMAKE_REQUIRED_INCLUDES ${ARGP_ROOT}/include)
endif()

# Source files
set(SEQUELIZER_SOURCES
    src/sequelizer_subcommands.c
    src/sequelizer_fast5.c
    src/sequelizer_convert.c
    src/sequelizer_plot.c
    src/sequelizer_seqgen.c
    src/core/fast5_io.c
    src/core/fast5_utils.c
    src/core/fast5_stats.c
    src/core/fast5_convert.c
    src/core/plot_utils.c
    src/core/seqgen_utils.c
    src/core/seq_tensor.c
    src/core/seq_utils.c
    src/core/util.c
)

# Sequelizer static library
add_library(sequelizer_static STATIC ${SEQUELIZER_SOURCES})
target_include_directories(sequelizer_static PRIVATE include src/core)

# Sequelizer executable
add_executable(sequelizer src/sequelizer.c)
target_include_directories(sequelizer PRIVATE include src/core)
target_link_libraries(sequelizer sequelizer_static m hdf5 argp)

# Install
install(TARGETS sequelizer RUNTIME DESTINATION bin)