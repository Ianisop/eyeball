#!/bin/bash

# Build directory
BUILD_DIR="build"

# Install prefix (per-user)
INSTALL_PREFIX="$HOME/.local"

# Create build directory if it doesn't exist
mkdir -p "$BUILD_DIR"

# Enter build directory
cd "$BUILD_DIR" || exit

# Run CMake with compilation database generation
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" ..

# Build the project
cmake --build .

# Move compile_commands.json to root
if [ -f compile_commands.json ]; then
    mv -f compile_commands.json ..
fi

# Install the executable to $INSTALL_PREFIX/bin
echo "Installing executable to $INSTALL_PREFIX/bin..."
make install

# Make sure $INSTALL_PREFIX/bin is in PATH
if [[ ":$PATH:" != *":$INSTALL_PREFIX/bin:"* ]]; then
    echo "Warning: $INSTALL_PREFIX/bin is not in your PATH"
    echo "You can add it with: export PATH=\"$INSTALL_PREFIX/bin:\$PATH\""
fi

# Run the executable in background
if [ $? -eq 0 ]; then
    echo "Build and install successful. Running the program..."
    "$INSTALL_PREFIX/bin/eyeball" &
else
    echo "Build failed."
fi

