#!/bin/bash
#
# QupZilla webOS SDK Setup Script
#
# This script extracts Qt 5.9.7 ARM libraries from the com.nizovn.qt5 IPK package
# and sets up the SDK for cross-compilation.
#
# Prerequisites:
#   - related-packages/com.nizovn.qt5_5.9.7-0_armv7.ipk must exist
#   - toolchains/gcc-linaro/ must be set up (see toolchains/README.md)
#
# Usage:
#   ./setup-sdk.sh
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SDK_LIB="${SCRIPT_DIR}/sdk/qt5-arm/lib"
IPK_FILE="${SCRIPT_DIR}/related-packages/com.nizovn.qt5_5.9.7-0_armv7.ipk"
TEMP_DIR="${SCRIPT_DIR}/.sdk-extract-tmp"

echo "========================================"
echo "QupZilla webOS SDK Setup"
echo "========================================"
echo ""

# Check prerequisites
if [ ! -f "${IPK_FILE}" ]; then
    echo "ERROR: Qt5 IPK not found at ${IPK_FILE}"
    echo "Please ensure related-packages/com.nizovn.qt5_5.9.7-0_armv7.ipk exists"
    exit 1
fi

if [ ! -d "${SCRIPT_DIR}/toolchains/gcc-linaro" ]; then
    echo "WARNING: Toolchain not found at toolchains/gcc-linaro/"
    echo "See toolchains/README.md for setup instructions"
    echo ""
fi

# Check if already set up
if [ -f "${SDK_LIB}/libQt5Core.so.5" ]; then
    echo "Qt5 libraries already exist in ${SDK_LIB}"
    read -p "Re-extract and overwrite? [y/N] " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Skipping extraction."
        exit 0
    fi
fi

echo "Extracting Qt5 libraries from IPK..."
echo "  Source: ${IPK_FILE}"
echo "  Target: ${SDK_LIB}"
echo ""

# Create temp directory
rm -rf "${TEMP_DIR}"
mkdir -p "${TEMP_DIR}"

# Extract IPK (it's an ar archive containing data.tar.gz)
cd "${TEMP_DIR}"
ar x "${IPK_FILE}"

# Extract data.tar.gz
tar xzf data.tar.gz

# Copy libraries to SDK
QT5_LIB_SRC="${TEMP_DIR}/usr/palm/applications/com.nizovn.qt5/lib"

if [ ! -d "${QT5_LIB_SRC}" ]; then
    echo "ERROR: Expected library path not found in IPK"
    echo "Looking for: ${QT5_LIB_SRC}"
    rm -rf "${TEMP_DIR}"
    exit 1
fi

echo "Copying Qt5 libraries..."
# Copy all .so.5 files (the actual libraries)
cp "${QT5_LIB_SRC}"/*.so.5 "${SDK_LIB}/"

# Count copied files
LIB_COUNT=$(ls -1 "${SDK_LIB}"/*.so.5 2>/dev/null | wc -l)
echo "  Copied ${LIB_COUNT} library files"

# Create .so symlinks pointing to .so.5 files
echo "Creating symlinks..."
cd "${SDK_LIB}"
for lib in *.so.5; do
    base="${lib%.so.5}"
    ln -sf "${lib}" "${base}.so"
done

# Create libQt5QmlModels.so symlink (Qt 5.14+ split this from Qt5Qml, but 5.9.7 doesn't have it)
# The build tools (Qt 5.15) expect it, so we point to Qt5Qml
ln -sf libQt5Qml.so libQt5QmlModels.so
ln -sf libQt5Qml.so.5 libQt5QmlModels.so.5
echo "  Created libQt5QmlModels.so -> libQt5Qml.so (compatibility symlink)"

# Create libGL.so symlink to PalmPDK's OpenGL ES
if [ -f "/opt/PalmPDK/device/lib/libGLESv2.so" ]; then
    ln -sf "/opt/PalmPDK/device/lib/libGLESv2.so" "${SDK_LIB}/libGL.so"
    echo "  Created libGL.so -> PalmPDK libGLESv2.so"
else
    echo "  WARNING: PalmPDK not found, libGL.so not created"
    echo "  You may need to manually create sdk/qt5-arm/lib/libGL.so"
fi

# Clean up
rm -rf "${TEMP_DIR}"

echo ""
echo "========================================"
echo "SDK Setup Complete"
echo "========================================"
echo ""
echo "Qt5 libraries extracted: ${LIB_COUNT}"
echo "Location: ${SDK_LIB}"
echo ""
echo "You can now build QupZilla with:"
echo "  ./build-arm.sh"
echo ""
