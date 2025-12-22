#!/bin/bash
#
# QupZilla Build and Package Script
#
# Builds QupZilla for webOS ARM and creates an IPK package.
#
# Usage:
#   ./build-and-package.sh
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SOURCE_BIN="${SCRIPT_DIR}/source/qupzilla/bin"
STAGING_DIR="${SCRIPT_DIR}/package-staging/com.nizovn.qupzilla"
VERSION="2.3.0"

echo "========================================"
echo "QupZilla Build and Package"
echo "========================================"
echo ""

# Step 1: Build
echo "Step 1: Building QupZilla..."
echo ""
"${SCRIPT_DIR}/build-arm.sh"

if [ $? -ne 0 ]; then
    echo "ERROR: Build failed"
    exit 1
fi

echo ""
echo "========================================"
echo "Step 2: Copying binaries to staging..."
echo "========================================"
echo ""

# Copy main binary
cp "${SOURCE_BIN}/qupzilla" "${STAGING_DIR}/bin/"
echo "  Copied qupzilla"

# Copy shared library
cp "${SOURCE_BIN}/libQupZilla.so.${VERSION}" "${STAGING_DIR}/bin/"
echo "  Copied libQupZilla.so.${VERSION}"

# Update symlinks
cd "${STAGING_DIR}/bin"
rm -f libQupZilla.so libQupZilla.so.2
ln -s "libQupZilla.so.${VERSION}" libQupZilla.so
ln -s "libQupZilla.so.${VERSION}" libQupZilla.so.2
echo "  Created library symlinks"

echo ""
echo "========================================"
echo "Step 3: Creating IPK package..."
echo "========================================"
echo ""

cd "${SCRIPT_DIR}/package-staging"
palm-package com.nizovn.qupzilla

IPK_FILE="${SCRIPT_DIR}/package-staging/com.nizovn.qupzilla_${VERSION}_all.ipk"

if [ ! -f "${IPK_FILE}" ]; then
    echo "ERROR: Package creation failed"
    exit 1
fi

echo ""
echo "========================================"
echo "Build and Package Complete"
echo "========================================"
echo ""
echo "Package: $(ls -lh "${IPK_FILE}" | awk '{print $9, $5}')"
echo ""

# Prompt for installation
read -p "Install to device with palm-install? [y/N] " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo ""
    echo "Installing to device..."
    palm-install "${IPK_FILE}"
    echo ""
    echo "Installation complete. Launch with: palm-launch com.nizovn.qupzilla"
fi
