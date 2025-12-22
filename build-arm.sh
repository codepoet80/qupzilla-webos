#!/bin/bash
# QupZilla ARM webOS Cross-Compilation Build Script
# Uses Linaro GCC 4.9 toolchain and Qt 5.9.7 headers/libraries
#
# Usage:
#   ./build-arm.sh          # Build
#   ./build-arm.sh clean    # Clean build output for fresh build
#   ./build-arm.sh --package # Build and package

set -e

# Paths
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
QUPZILLA_SRC="${SCRIPT_DIR}/source/qupzilla"
BUILD_DIR="${SCRIPT_DIR}/build-arm"

# Handle "clean" argument
if [ "$1" == "clean" ]; then
    echo "=== Cleaning build output ==="
    echo ""
    if [ -d "${BUILD_DIR}" ]; then
        echo "Removing ${BUILD_DIR}..."
        rm -rf "${BUILD_DIR}"
    fi
    if [ -d "${QUPZILLA_SRC}/bin" ]; then
        echo "Removing ${QUPZILLA_SRC}/bin..."
        rm -rf "${QUPZILLA_SRC}/bin"
    fi
    if [ -d "${QUPZILLA_SRC}/build" ]; then
        echo "Removing ${QUPZILLA_SRC}/build..."
        rm -rf "${QUPZILLA_SRC}/build"
    fi
    echo ""
    echo "Clean complete."
    exit 0
fi

echo "=== QupZilla ARM webOS Build ==="
echo ""

SDK_DIR="${SCRIPT_DIR}/sdk/qt5-arm"

# Linaro GCC 4.9.4 toolchain (compatible with Qt 5.9.7)
LINARO_GCC="${SCRIPT_DIR}/toolchains/gcc-linaro"
ARM_BIN="${LINARO_GCC}/bin"
ARM_PREFIX="arm-linux-gnueabi"

# Device headers from webOS SDK
DEVICE_INCLUDE="${SCRIPT_DIR}/source/qt5-webos-sdk/files/device/include"
DEVICE_LIB="${SCRIPT_DIR}/source/qt5-webos-sdk/files/device/lib"

# PalmPDK
PALM_PDK="/opt/PalmPDK"

# webOS application paths (where app runs on device)
APP_RUNPATH="/media/cryptofs/apps/usr/palm/applications/com.nizovn.qupzilla"
QT5_RUNPATH="/media/cryptofs/apps/usr/palm/applications/com.nizovn.qt5/lib"
GLIBC_RUNPATH="/media/cryptofs/apps/usr/palm/applications/com.nizovn.glibc/lib"
OPENSSL_RUNPATH="/media/cryptofs/apps/usr/palm/applications/com.nizovn.openssl/lib"

# Check prerequisites
echo "Checking prerequisites..."

if [ ! -d "${LINARO_GCC}" ]; then
    echo "ERROR: Linaro toolchain not found at ${LINARO_GCC}"
    echo "Please extract a compatible gcc-linaro toolchain to toolchains/gcc-linaro/"
    exit 1
fi

if [ ! -d "${SDK_DIR}/lib" ]; then
    echo "ERROR: Qt5 ARM SDK not found at ${SDK_DIR}"
    exit 1
fi

if [ ! -d "${QUPZILLA_SRC}" ]; then
    echo "ERROR: QupZilla source not found at ${QUPZILLA_SRC}"
    exit 1
fi

echo "  Linaro GCC: ${LINARO_GCC}"
echo "  Qt5 ARM SDK: ${SDK_DIR}"
echo "  QupZilla source: ${QUPZILLA_SRC}"
echo ""

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Export build configuration for QupZilla
export PORTABLE_BUILD="true"
export NO_X11="true"
export USE_LIBPATH="${APP_RUNPATH}"
export SHARE_FOLDER="${APP_RUNPATH}"
export QUPZILLA_PREFIX="${APP_RUNPATH}"
export NO_SYSTEM_DATAPATH="true"
export DISABLE_DBUS="true"

# Set up cross-compilation environment
export PATH="${ARM_BIN}:${PATH}"
export CC="${ARM_BIN}/${ARM_PREFIX}-gcc"
export CXX="${ARM_BIN}/${ARM_PREFIX}-g++"
export LD="${ARM_BIN}/${ARM_PREFIX}-ld"
export AR="${ARM_BIN}/${ARM_PREFIX}-ar"
export RANLIB="${ARM_BIN}/${ARM_PREFIX}-ranlib"
export STRIP="${ARM_BIN}/${ARM_PREFIX}-strip"

# Include paths - Qt 5.9.7 headers from our SDK
QT_INCLUDES="-I${SDK_DIR}/include"
QT_INCLUDES="${QT_INCLUDES} -I${SDK_DIR}/include/QtCore"
QT_INCLUDES="${QT_INCLUDES} -I${SDK_DIR}/include/QtGui"
QT_INCLUDES="${QT_INCLUDES} -I${SDK_DIR}/include/QtWidgets"
QT_INCLUDES="${QT_INCLUDES} -I${SDK_DIR}/include/QtNetwork"
QT_INCLUDES="${QT_INCLUDES} -I${SDK_DIR}/include/QtSql"
QT_INCLUDES="${QT_INCLUDES} -I${SDK_DIR}/include/QtPrintSupport"
QT_INCLUDES="${QT_INCLUDES} -I${SDK_DIR}/include/QtWebEngineWidgets"
QT_INCLUDES="${QT_INCLUDES} -I${SDK_DIR}/include/QtWebEngineCore"
QT_INCLUDES="${QT_INCLUDES} -I${SDK_DIR}/include/QtWebChannel"
QT_INCLUDES="${QT_INCLUDES} -I${SDK_DIR}/include/QtQuick"
QT_INCLUDES="${QT_INCLUDES} -I${SDK_DIR}/include/QtQuickWidgets"
QT_INCLUDES="${QT_INCLUDES} -I${SDK_DIR}/include/QtQml"

# Device includes (OpenGL ES, OpenSSL, etc.)
DEVICE_INCLUDES="-I${DEVICE_INCLUDE} -I${PALM_PDK}/include"

# Compiler flags
CFLAGS="-march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp -O2"
CXXFLAGS="${CFLAGS} -std=gnu++11"

# Linker flags
LDFLAGS="-L${SDK_DIR}/lib"
LDFLAGS="${LDFLAGS} -L${DEVICE_LIB}"
LDFLAGS="${LDFLAGS} -L${PALM_PDK}/device/lib"
LDFLAGS="${LDFLAGS} -Wl,-rpath-link,${SDK_DIR}/lib"
LDFLAGS="${LDFLAGS} -Wl,-rpath,${QT5_RUNPATH}"
LDFLAGS="${LDFLAGS} -Wl,-rpath,${GLIBC_RUNPATH}"
LDFLAGS="${LDFLAGS} -Wl,-rpath,${OPENSSL_RUNPATH}"
LDFLAGS="${LDFLAGS} -Wl,--dynamic-linker=${GLIBC_RUNPATH}/ld.so"
LDFLAGS="${LDFLAGS} -Wl,--allow-shlib-undefined"

echo "Configuring with qmake..."

# Use our custom qmake with qt.conf
QMAKE="${SDK_DIR}/bin/qmake"

${QMAKE} "${QUPZILLA_SRC}/QupZilla.pro" \
    -spec linux-webos-arm-g++ \
    "DEFINES+=NO_X11 PORTABLE_BUILD DISABLE_DBUS NO_SYSTEM_DATAPATH"

echo ""
echo "Building..."
make -j$(nproc) 2>&1 | tee build.log

echo ""
if [ -f "${QUPZILLA_SRC}/bin/qupzilla" ]; then
    echo "Build complete!"
    file "${QUPZILLA_SRC}/bin/qupzilla"
    readelf -l "${QUPZILLA_SRC}/bin/qupzilla" | grep -A2 "interpreter" || true

    # Verify OpenSSL version (must be 1.0.0, not 0.9.8)
    echo ""
    echo "Checking OpenSSL linkage..."
    CRYPTO_VERSION=$(readelf -d "${QUPZILLA_SRC}/bin/libQupZilla.so.2" | grep libcrypto | head -1)
    if echo "${CRYPTO_VERSION}" | grep -q "0.9.8"; then
        echo "WARNING: Linked against libcrypto.so.0.9.8 - this will crash on device!"
        echo "Ensure source/qt5-webos-sdk/files/device/lib/ contains OpenSSL 1.0.0"
    elif echo "${CRYPTO_VERSION}" | grep -q "1.0.0"; then
        echo "OK: Linked against libcrypto.so.1.0.0"
    else
        echo "OpenSSL linkage: ${CRYPTO_VERSION}"
    fi
else
    echo "Build may have failed - checking for library..."
    if [ -f "${QUPZILLA_SRC}/bin/libQupZilla.so.2" ]; then
        echo "Library built: ${QUPZILLA_SRC}/bin/libQupZilla.so.2"
        file "${QUPZILLA_SRC}/bin/libQupZilla.so.2"
    else
        echo "Build failed - check build.log"
        exit 1
    fi
fi

# Packaging step (optional - run with --package flag)
if [ "$1" == "--package" ]; then
    echo ""
    echo "=== Packaging for webOS ==="

    # Get version from defines.pri
    VERSION=$(grep "QZ_VERSION" "${QUPZILLA_SRC}/src/defines.pri" | head -1 | sed 's/.*= //')
    echo "Version: ${VERSION}"

    STAGING_DIR="${SCRIPT_DIR}/package-staging/com.nizovn.qupzilla"

    # Copy binaries
    cp "${QUPZILLA_SRC}/bin/qupzilla" "${STAGING_DIR}/bin/"
    cp "${QUPZILLA_SRC}/bin/libQupZilla.so.${VERSION}" "${STAGING_DIR}/bin/"

    # Update symlinks
    cd "${STAGING_DIR}/bin"
    rm -f libQupZilla.so libQupZilla.so.2
    ln -s "libQupZilla.so.${VERSION}" libQupZilla.so
    ln -s "libQupZilla.so.${VERSION}" libQupZilla.so.2

    # Create IPK
    cd "${SCRIPT_DIR}/package-staging"
    palm-package com.nizovn.qupzilla

    echo ""
    echo "Package created: $(ls -1 *.ipk | tail -1)"
fi
