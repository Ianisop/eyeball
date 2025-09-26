# Minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(eyeball VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the executable
# Replace main.cpp with your source files
add_executable(eyeball
  src/main.cpp
)

target_compile_options(eyeball PRIVATE -Wall -Wextra)

