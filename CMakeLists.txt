cmake_minimum_required(VERSION 3.23)
project(Sequelizer VERSION 0.1.0)

set(CMAKE_C_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

# Sequelizer executable
add_executable(sequelizer src/sequelizer.c)
target_include_directories(sequelizer PRIVATE include)
target_link_libraries(sequelizer m)  # Just math lib for now

# Install
install(TARGETS sequelizer RUNTIME DESTINATION bin)