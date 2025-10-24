#!/bin/bash
# Simple build script for BNO085 project

# Exit if any command fails
set -e

# Create build directory if it doesn't exist
mkdir -p build

# Compile the project
g++ -Iinclude src/bno085.cpp src/util.cpp read_bno085.cpp -o build/read_bno085

echo "Build complete: ./build/read_bno085"
