cmake_minimum_required(VERSION 3.23)
project(Sequelizer VERSION 0.1.0)

set(CMAKE_C_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

# Source files
set(SEQUELIZER_SOURCES
    src/sequelizer_subcommands.c
)

# Sequelizer static library
add_library(sequelizer_static STATIC ${SEQUELIZER_SOURCES})
target_include_directories(sequelizer_static PRIVATE include)

# Sequelizer executable
add_executable(sequelizer src/sequelizer.c)
target_include_directories(sequelizer PRIVATE include)
target_link_libraries(sequelizer sequelizer_static m)  # Link static lib + math lib

# Install
install(TARGETS sequelizer RUNTIME DESTINATION bin)