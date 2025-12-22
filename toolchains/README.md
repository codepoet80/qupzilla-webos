# Toolchains

This directory contains cross-compilation toolchains required for building QupZilla for webOS. The actual toolchain binaries are not committed to the repository due to their size.

## Required Toolchains

### gcc-linaro/

**Linaro GCC ARM Cross-Compiler**

| Property | Value |
|----------|-------|
| Version | 4.9.4 (Linaro GCC 4.9-2017.01) |
| Target | arm-linux-gnueabi |
| Host | x86_64-linux-gnu |
| Download | [Linaro Releases](https://releases.linaro.org/components/toolchain/binaries/4.9-2017.01/arm-linux-gnueabi/) |
| Filename | `gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabi.tar.xz` |
| Size | ~430 MB (extracted) |

#### Installation

```bash
cd toolchains/
wget https://releases.linaro.org/components/toolchain/binaries/4.9-2017.01/arm-linux-gnueabi/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabi.tar.xz
tar xf gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabi.tar.xz
mv gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabi gcc-linaro
rm gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabi.tar.xz
```

#### Verification

After installation, verify the toolchain works:

```bash
./gcc-linaro/bin/arm-linux-gnueabi-gcc --version
# Should output: arm-linux-gnueabi-gcc (Linaro GCC 4.9-2017.01) 4.9.4
```

## Compatibility Notes

- **GCC 4.8.x** toolchains should also work (Qt 5.9.7 was originally built with 4.8)
- **GCC 4.9.x** is confirmed compatible and recommended
- Later GCC versions (5.x+) may have ABI incompatibilities with the Qt 5.9.7 runtime on the device
- The toolchain must target `arm-linux-gnueabi` (soft-float ABI), not `arm-linux-gnueabihf`
