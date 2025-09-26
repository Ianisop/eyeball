#!/bin/bash

# Build directory
BUILD_DIR="build"

# Create build directory if it doesn't exist
mkdir -p "$BUILD_DIR"

# Enter build directory
cd "$BUILD_DIR" || exit

# Run CMake with compilation database generation
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..

# Build the project
cmake --build .

# Move compile_commands.json to root
if [ -f compile_commands.json ]; then
    mv -f compile_commands.json ..
fi

# Run the executable 
if [ $? -eq 0 ]; then
    echo "Build successful. Running the program..."
    ./eyeball
else
    echo "Build failed."
fi

