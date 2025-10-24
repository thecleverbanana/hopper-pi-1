#!/bin/bash
# Simple build script for LoadCell project using libgpiod

# Exit immediately if any command fails
set -e

# Create build directory if it doesn't exist
mkdir -p build

# Compile and link with libgpiod
g++ -Iinclude src/loadcell.cpp read_loadcell.cpp -o build/read_loadcell -lgpiod

echo "Build complete: ./build/read_loadcell"
