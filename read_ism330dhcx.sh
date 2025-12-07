#!/bin/bash
# Build script for ISM330DHCX test program

# Exit if any command fails
set -e

# Create build directory if it doesn't exist
mkdir -p build

# Compile the project
gcc -I. ism330dhcx-pid/ism330dhcx_reg.c read_ism330dhcx.c -o build/read_ism330dhcx -lm

echo "Build complete: ./build/read_ism330dhcx"
echo ""
echo "Usage: ./build/read_ism330dhcx [i2c_bus]"
echo "  Default: i2c_bus=1"
echo "  Reads from two IMUs: 0x6A and 0x6B"
echo "  Data rate: 1666 Hz (closest to 1000 Hz)"
echo "  Example: ./build/read_ism330dhcx 1"

