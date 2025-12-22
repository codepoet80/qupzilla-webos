# QupZilla for webOS

QupZilla 2.3.0 web browser ported to legacy webOS (HP TouchPad / Pre3). Uses Qt 5.9.7 runtime with Qt 5.15 build tools, cross-compiled for ARMv7.

## Prerequisites

### Host System
- Ubuntu 24.04 LTS (or similar Linux distribution), x86_64
- Palm PDK installed at `/opt/PalmPDK/`

### System Packages

```bash
sudo apt install \
    build-essential \
    qtbase5-dev-tools \
    qt5-qmake \
    pkg-config \
    nodejs \
    npm
```

### webOS Tools

Install the SDK from: https://sdk.webosarchive.org

This provides the command line tools, like `palm-package` and `palm-launch`, as well as device tools like `novaterm`.

Paths expected include: 
- `/opt/PalmSDK` 
- `/opt/PalmPDK`

## Quick Start

### 1. Set Up Toolchain

Download and extract the Linaro GCC ARM cross-compiler:

```bash
cd toolchains/
wget https://releases.linaro.org/components/toolchain/binaries/4.9-2017.01/arm-linux-gnueabi/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabi.tar.xz
tar xf gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabi.tar.xz
mv gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabi gcc-linaro
rm gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabi.tar.xz
```

### 2. Set Up SDK Libraries

Extract Qt5 ARM libraries from the IPK package:

```bash
./setup-sdk.sh
```

### 3. Build

```bash
./build-arm.sh
```

Or manually:

```bash
mkdir -p build-arm && cd build-arm
../sdk/qt5-arm/bin/qmake ../source/qupzilla/QupZilla.pro \
    -spec linux-webos-arm-g++ \
    "DEFINES+=NO_X11 PORTABLE_BUILD DISABLE_DBUS NO_SYSTEM_DATAPATH"
make -j$(nproc)
```

### 4. Package and Deploy

```bash
# Copy binaries to staging
cp source/qupzilla/bin/qupzilla package-staging/com.nizovn.qupzilla/bin/
cp source/qupzilla/bin/libQupZilla.so.2.3.0 package-staging/com.nizovn.qupzilla/bin/
cd package-staging/com.nizovn.qupzilla/bin
rm -f libQupZilla.so libQupZilla.so.2
ln -s libQupZilla.so.2.3.0 libQupZilla.so
ln -s libQupZilla.so.2.3.0 libQupZilla.so.2

# Create and install package
cd ../..
palm-package com.nizovn.qupzilla
palm-install com.nizovn.qupzilla_2.3.0_all.ipk
```

## Device Requirements

Install these packages on the webOS device (available in `related-packages/`):

| Package | Purpose |
|---------|---------|
| `com.nizovn.glibc` | Modern glibc |
| `com.nizovn.qt5` | Qt 5.9.7 runtime |
| `com.nizovn.qt5qpaplugins` | QPA plugins for webOS |
| `com.nizovn.qt5sdk` | Jailer wrapper for Qt5 apps |
| `com.nizovn.openssl` | OpenSSL 1.0.2p |
| `com.nizovn.cacert` | CA certificates |
| `org.webosinternals.dbus` | D-Bus |

## Project Structure

```
├── build-arm/             # Binary Build output directory
├── package-staging/       # IPK staging and packaging area
├── related-packages/      # Pre-built device packages
├── sdk/qt5-arm/           # Qt 5.15 build tools + Qt 5.9.7 headers/libs
├── source/qupzilla/       # QupZilla browser source (main code)
└── toolchains/gcc-linaro/ # Linaro GCC 4.9.4 ARM cross-compiler
```

## Troubleshooting

### "cannot find -lQt5QmlModels"
Run `./setup-sdk.sh` to create the compatibility symlink.

### "undefined reference to _M_widen_init"
Ensure `sdk/qt5-arm/lib/libctype_stub.a` exists and is being linked.

### App crashes on device
1. Verify all device packages are installed
2. Check `readelf -d libQupZilla.so.2 | grep libcrypto` shows `1.0.0`, not `0.9.8`

## Documentation

- **CLAUDE.md** - Detailed technical documentation, architecture notes, and webOS-specific modifications
- **toolchains/README.md** - Toolchain setup details

## Build Architecture

This project uses a hybrid build approach:
- **Build tools**: Qt 5.15 (qmake, moc, rcc, uic) generate code
- **Target runtime**: Qt 5.9.7 libraries on the device
- **Compiler**: Linaro GCC 4.9.4 for ARMv7 cross-compilation

Stub libraries (`libctype_stub.a`, `libqt_resource_stub.a`) provide ABI compatibility between the Qt 5.15 build tools and Qt 5.9.7 runtime.
