#!/bin/bash
# Simple build script for BNO085 project

# Exit if any command fails
set -e

# Create build directory if it doesn't exist
mkdir -p build

# Compile the project
g++ -Iinclude src/loadcell.cpp read_loadcell.cpp -o build/read_loadcell

echo "Build complete: ./build/read_bno085"
