#!/bin/bash

# Banking System - Quick Build Script
# This script automates the build process

set -e  # Exit on any error

echo "════════════════════════════════════════════════════════"
echo "  Banking Transaction Processing System"
echo "  Quick Build Script"
echo "════════════════════════════════════════════════════════"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check for required tools
echo "Checking dependencies..."

if ! command -v g++ &> /dev/null; then
    echo -e "${RED}✗ g++ not found${NC}"
    echo "Install with: sudo apt-get install g++"
    exit 1
fi
echo -e "${GREEN}✓ g++ found${NC}"

if ! command -v cmake &> /dev/null; then
    echo -e "${RED}✗ cmake not found${NC}"
    echo "Install with: sudo apt-get install cmake"
    exit 1
fi
echo -e "${GREEN}✓ cmake found${NC}"

if ! command -v make &> /dev/null; then
    echo -e "${RED}✗ make not found${NC}"
    echo "Install with: sudo apt-get install build-essential"
    exit 1
fi
echo -e "${GREEN}✓ make found${NC}"

echo ""
echo "All dependencies satisfied!"
echo ""

# Clean previous build
if [ -d "build" ]; then
    echo -e "${YELLOW}Cleaning previous build...${NC}"
    rm -rf build
fi

# Create build directory
echo "Creating build directory..."
mkdir build
cd build

# Configure with CMake
echo ""
echo "Configuring project with CMake..."
cmake .. || {
    echo -e "${RED}✗ CMake configuration failed${NC}"
    exit 1
}

# Build
echo ""
echo "Building project..."
make || {
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
}

# Success message
echo ""
echo "════════════════════════════════════════════════════════"
echo -e "${GREEN}✓ Build successful!${NC}"
echo "════════════════════════════════════════════════════════"
echo ""
echo "To run the program:"
echo "  cd build"
echo "  ./banking_system"
echo ""
echo "Or simply run:"
echo "  ./build/banking_system"
echo ""
