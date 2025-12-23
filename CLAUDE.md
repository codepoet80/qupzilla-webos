# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

QupZilla 2.3.0 web browser ported to legacy webOS (HP TouchPad / Pre3, ARMv7 Cortex-A8 from 2011). The browser uses Qt 5.9.7 runtime with Qt 5.15 build tools, cross-compiled for webOS.

## Repository Structure

- `source/qupzilla/` - QupZilla browser source (C++/Qt, the main code to modify)
- `source/qt5-webos-sdk/` - Build instructions and patches for Qt 5.9.7 cross-compilation
- `source/qt5-qpa-webos-plugin/` - Qt Platform Abstraction plugin for webOS display/input
- `source/com.nizovn.qt5sdk/` - webOS jailer wrapper for Qt5 apps
- `sdk/qt5-arm/` - Pre-built Qt 5.15 ARM SDK with Qt 5.9.7 compatible runtime
- `toolchains/gcc-linaro/` - Linaro GCC ARM cross-compiler toolchain
- `build-arm/` - Build output directory for ARM cross-compilation
- `package-staging/` - Staging area for webOS IPK package creation
- `related-packages/` - Pre-built .ipk packages for webOS installation

## Build Environment

The build uses a hybrid approach:
- **Build tools**: Qt 5.15 (qmake, moc, rcc, uic) from `sdk/qt5-arm/bin/`
- **Target runtime**: Qt 5.9.7 libraries on device
- **Compiler**: Linaro GCC 4.9.4 (`toolchains/gcc-linaro/`)

### Key SDK Files
- `sdk/qt5-arm/bin/` - Qt 5.15 build tools (host)
- `sdk/qt5-arm/lib/` - Qt 5.9.7 ARM libraries (target)
- `sdk/qt5-arm/mkspecs/linux-webos-arm-g++/` - Cross-compilation mkspec

## Runtime Dependencies (webOS Device)

All packages in `related-packages/` must be installed on the device:
- `com.nizovn.glibc` - Modern glibc (webOS ships ancient one)
- `com.nizovn.qt5` - Qt 5.9.7 runtime libraries
- `com.nizovn.qt5qpaplugins` - QPA plugins for webOS
- `com.nizovn.qt5sdk` - Jailer wrapper enabling Qt5 apps
- `com.nizovn.openssl` - OpenSSL 1.0.2p
- `com.nizovn.cacert` - CA certificates at `/media/cryptofs/apps/usr/palm/applications/com.nizovn.cacert/files/cert.pem`
- `org.webosinternals.dbus` - D-Bus

## Building QupZilla for webOS

### Using Build Scripts (Recommended)

```bash
# Build only
./build-arm.sh

# Build and package (prompts for device install)
./build-and-package.sh

# Clean build output for fresh build
./build-arm.sh clean
```

### Manual Build

```bash
# Create build directory
mkdir -p build-arm && cd build-arm

# Run qmake with webOS mkspec
../sdk/qt5-arm/bin/qmake ../source/qupzilla/QupZilla.pro \
  -spec linux-webos-arm-g++ \
  "DEFINES+=NO_X11 PORTABLE_BUILD DISABLE_DBUS NO_SYSTEM_DATAPATH"

# Build
make -j$(nproc)
```

### Packaging for webOS

```bash
# Copy binaries to staging (update version number as needed)
cp source/qupzilla/bin/qupzilla package-staging/com.nizovn.qupzilla/bin/
cp source/qupzilla/bin/libQupZilla.so.2.3.0 package-staging/com.nizovn.qupzilla/bin/

# Update library symlinks
cd package-staging/com.nizovn.qupzilla/bin
rm -f libQupZilla.so libQupZilla.so.2
ln -s libQupZilla.so.2.3.0 libQupZilla.so
ln -s libQupZilla.so.2.3.0 libQupZilla.so.2

# Create IPK
cd ../..
palm-package com.nizovn.qupzilla
```

## webOS appinfo.json Configuration

The working `appinfo.json` configuration (keep minimal exports):
```json
{
  "title": "QupZilla",
  "id": "com.nizovn.qupzilla",
  "version": "2.3.0",
  "type": "pdk",
  "main": "bin/qupzilla",
  "icon": "qupzilla.png",
  "keywords": ["browser"],
  "qt5sdk": {
    "exports": [
      "QT_QPA_WEBOS_AUTOROTATE=1",
      "QT_QPA_WEBOS_RIGHT_CLICK_ON_LONG_TAP=1"
    ]
  },
  "requiredMemory": 150
}
```

**Important**: Keep `qt5sdk.exports` minimal. GPU and rendering configuration is set in `main.cpp` via `QTWEBENGINE_CHROMIUM_FLAGS`. Do NOT set `QMLSCENE_DEVICE` or `QT_QUICK_BACKEND` here as they conflict with GPU acceleration.

## Key Modifications Made for webOS

### 1. Environment Variables (main.cpp)
Set before QApplication creation for PORTABLE_BUILD on Linux:
- `QTWEBENGINE_CHROMIUM_FLAGS` - GPU acceleration flags (see below)
- `QT_QPA_FONTDIR=/usr/share/fonts` - Font discovery via fontconfig
- `QT_QPA_WEBOS_PHYSICAL_WIDTH=197` / `HEIGHT=148` - Physical screen size (mm)
- `SSL_CERT_FILE=/media/cryptofs/apps/usr/palm/applications/com.nizovn.cacert/files/cert.pem`

**GPU Acceleration Configuration** (PowerVR SGX540):
```cpp
qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
    "--use-gl=egl "                         // OpenGL ES via EGL
    "--enable-gpu-rasterization "           // GPU page painting
    "--enable-native-gpu-memory-buffers "   // Efficient GPU memory
    "--num-raster-threads=2 "               // Parallel rasterization (dual-core)
    "--disable-background-timer-throttling" // Responsive JS timers
);
```

### 2. OpenGL ES 2 Linking
Changed `sdk/qt5-arm/mkspecs/modules/qt_lib_gui_private.pri`:
- `QMAKE_LIBS_OPENGL = -lGLESv2` (was `-lGL`)

### 3. Start Page Image Fix (qupzillaschemehandler.cpp)
Fixed `%ABOUT-IMG%` not loading on qupzilla:start, qupzilla:about, qupzilla:config pages.
Original code used `QzTools::pixmapToDataUrl()` which requires image format plugins.
Changed to read PNG bytes directly and base64 encode:
```cpp
QString imgDataUrl = QSL("data:image/png;base64,") +
  QString::fromLatin1(QzTools::readAllFileByteContents(QSL(":icons/other/startpage.png")).toBase64());
```

### 4. Resource Format Compatibility (qmake.conf)
Added for Qt 5.15 rcc to generate Qt 5.9.7 compatible resources:
```
QMAKE_RESOURCE_FLAGS += --no-compress --format-version 1
```

### 5. ABI Compatibility Stubs
Link against stub libraries for GCC 5 ABI compatibility:
- `libctype_stub.so` - Provides `_M_widen_init`
- `libqt_resource_stub.so` - Provides `qt_resourceFeatureZlib`

### 6. OpenSSL 1.0.0 (CRITICAL)
The build links against OpenSSL. The PalmPDK provides `libcrypto.so.0.9.8` but the device needs `libcrypto.so.1.0.0`. The `source/qt5-webos-sdk/files/device/lib/` directory contains the correct OpenSSL 1.0.0 libraries which are added to the link path before `/opt/PalmPDK/device/lib` in the mkspec. Verify with `readelf -d` that the binary links to `libcrypto.so.1.0.0`, not `libcrypto.so.0.9.8`.

### 7. SSL Certificate Initialization (main.cpp)
After MainApplication creation, SSL certificates must be loaded:
```cpp
#if defined(PORTABLE_BUILD) && defined(Q_OS_LINUX)
    QList<QSslCertificate> systemCerts = QSslConfiguration::systemCaCertificates();
    if (!systemCerts.isEmpty()) {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setCaCertificates(systemCerts);
        QSslConfiguration::setDefaultConfiguration(sslConfig);
    }
#endif
```
**Important**: Use `QSslConfiguration::systemCaCertificates()` NOT `QSslCertificate::fromPath()` - the latter requires the certificate file to be in PEM format with specific parsing that may fail.

### 8. Menu Cleanup for Tablet (mainmenu.cpp)
The hamburger menu (`initSuperMenu`) has desktop-oriented items hidden for PORTABLE_BUILD using `#ifndef PORTABLE_BUILD`:
- Sessions submenu
- Session Manager
- Print
- Help menu

### 9. Deferred WebView Creation (PORTABLE_BUILD)

In PORTABLE_BUILD, WebView creation is deferred to avoid initialization order issues with QtWebEngine:

**The Problem**: `TabWidget::addView()` calls `insertTab()` which triggers `currentChanged` signal, which calls `BrowserWindow::currentTabChanged()`. But at this point, `WebTab::attach()` hasn't run yet, so there's no WebView.

**The Solution** (in `tabwidget.cpp`):
```cpp
// After attach() creates the WebView, refresh NavigationBar directly
if (currentIndex() == index && webTab->webView()) {
    m_window->navigationBar()->setCurrentView(webTab->webView());
}
```

**Critical**: Do NOT call full `currentTabChanged()` here. That function includes `setTabOrder()` which crashes during early Qt initialization. Only call what's needed: `navigationBar()->setCurrentView()`.

### 10. Qt Signal/Slot Gotchas

**SLOT() macro only works with declared slots**: The `SLOT(functionName())` macro only works with functions declared in the `public slots:`, `protected slots:`, or `private slots:` sections of a class. Regular public functions will silently fail.

```cpp
// WRONG - currentTabChanged() is not a slot, this silently does nothing
QTimer::singleShot(0, m_window, SLOT(currentTabChanged()));

// CORRECT - use a lambda instead
QTimer::singleShot(0, m_window, [window]() {
    window->currentTabChanged();
});
```

**Defensive null checks**: When code can be called during deferred initialization, always check that UI elements exist:
```cpp
void BrowserWindow::currentTabChanged()
{
    if (!m_navigationToolbar) {
        return;  // UI not ready yet
    }
    // ... rest of function
}
```

### 11. webOS PDK Launch Parameters

PDK apps on webOS receive launch parameters via command-line arguments (`argv`), not via Luna Service Bus callbacks like web apps.

**How webOS passes parameters:**
```bash
# Launching from another app or command line:
luna-send -n 1 -f luna://com.palm.applicationManager/launch \
  '{ "id": "com.nizovn.qupzilla", "params": ["http://example.com"] }'
```

The app receives this as `argv[1]` in JSON array format with escaped forward slashes:
```
argv[1] = [ "http:\/\/example.com" ]
```

**Parsing the URL (main.cpp):**
```cpp
// Check if argument is a JSON array (webOS format)
if (arg.startsWith(QLatin1String("[")) && arg.contains(QLatin1String("\\/"))) {
    int firstQuote = arg.indexOf('"');
    int lastQuote = arg.lastIndexOf('"');
    if (firstQuote != -1 && lastQuote > firstQuote) {
        QString url = arg.mid(firstQuote + 1, lastQuote - firstQuote - 1);
        url.replace(QLatin1String("\\/"), QLatin1String("/"));  // Unescape JSON
        launchUrl = url;
    }
}
```

### 12. Event-Driven Initialization (CRITICAL for PORTABLE_BUILD)

**Problem**: PORTABLE_BUILD defers QtWebEngine initialization to avoid blocking the UI. Timer-based delays (e.g., `QTimer::singleShot(5000, ...)`) are unreliable because initialization time varies.

**Solution**: Use the `BrowserWindow::startingCompleted()` signal which fires when the window is fully ready.

**Implementation pattern:**
```cpp
// In mainapplication.h - add static cache and slot:
static void setPendingLaunchUrl(const QString &url);
static QString pendingLaunchUrl();
// private slots:
void onWindowStartingCompleted();

// In main.cpp - cache URL before MainApplication creation:
MainApplication::setPendingLaunchUrl(launchUrl);

// In mainapplication.cpp - connect signal when creating window:
connect(window, SIGNAL(startingCompleted()), this, SLOT(onWindowStartingCompleted()));

// In the slot - safely open the URL:
void MainApplication::onWindowStartingCompleted()
{
    if (!s_pendingLaunchUrl.isEmpty()) {
        BrowserWindow* window = getWindow();
        if (window) {
            window->loadAddress(QUrl::fromUserInput(s_pendingLaunchUrl));
        }
        s_pendingLaunchUrl.clear();
    }
}
```

**Key insight**: Never use timers for initialization-dependent actions in PORTABLE_BUILD. Always use signals that indicate actual readiness.

### 13. Adding Menu Items to SuperMenu (Hamburger Menu)

The SuperMenu is built in `MainMenu::initSuperMenu()` in `mainmenu.cpp`. To add new items:

1. **Create actions in `init()`** (so they're available for both menus):
```cpp
action = new QAction(tr("Load Images"), this);
action->setCheckable(true);
action->setChecked(mApp->webSettings()->testAttribute(QWebEngineSettings::AutoLoadImages));
connect(action, &QAction::toggled, this, &MainMenu::toggleLoadImages);
m_actions[QSL("Tools/LoadImages")] = action;
```

2. **Add to SuperMenu in `initSuperMenu()`**:
```cpp
superMenu->addAction(m_actions[QSL("Tools/LoadImages")]);

// For submenus:
QMenu* userAgentMenu = new QMenu(tr("User Agent"));
connect(userAgentMenu, &QMenu::aboutToShow, this, &MainMenu::aboutToShowUserAgentMenu);
superMenu->addMenu(userAgentMenu);
```

3. **Use `#ifndef PORTABLE_BUILD`** to hide desktop-only items on webOS.

### 14. User-Agent Management

QupZilla has built-in `UserAgentManager` infrastructure. To add a user-agent switcher menu:

1. Store selected UA in settings under `"Web-Browser-Settings/UserAgent"`
2. Apply via `mApp->webProfile()->setHttpUserAgent(ua)`
3. Use `QActionGroup` for radio-button behavior in submenu
4. Populate menu dynamically in `aboutToShow` slot to reflect current state

## QupZilla Architecture

### Core Library (`src/lib/`)
Shared library `libQupZilla` containing:
- **app/** - `MainApplication` singleton (access via `mApp` macro), `BrowserWindow`
- **webengine/** - QtWebEngine integration (`WebPage`, `WebView`)
- **navigation/** - URL bar, location completer
- **tabwidget/** - Tab models (`TabModel`, `TabMruModel`, `TabTreeModel`)
- **bookmarks/**, **history/** - Data management
- **adblock/** - Built-in ad blocking
- **plugins/** - Plugin infrastructure

### Plugin System (`src/plugins/`)
Plugins implement `PluginInterface`. Available: AutoScroll, FlashCookieManager, GreaseMonkey, ImageFinder, MouseGestures, PIM, StatusBarIcons, TabManager, VerticalTabs

## Testing (Host)

For development testing on the host machine (not cross-compiled):
```bash
cd source/qupzilla
qmake CONFIG+=debug -r
make
cd tests/autotests && qmake && make && ./autotests
```

## Troubleshooting

### App crashes immediately on device
1. **Check OpenSSL version**: Run `readelf -d libQupZilla.so.2 | grep libcrypto`. If it shows `0.9.8`, the build linked against the wrong OpenSSL from PalmPDK. Ensure `source/qt5-webos-sdk/files/device/lib/` contains OpenSSL 1.0.0 and is listed in LIBS before PalmPDK.

2. **Check device packages**: Ensure all runtime dependencies are installed: com.nizovn.glibc, com.nizovn.qt5, com.nizovn.qt5qpaplugins, com.nizovn.qt5sdk, com.nizovn.openssl, com.nizovn.cacert

3. **Font/EGL errors**: These are usually caused by missing environment variables. Ensure main.cpp sets `QT_QPA_FONTDIR` and other variables for PORTABLE_BUILD.

4. **"QMLSCENE_DEVICE=softwarecontext"**: this setting MUST be in appinfo.json in the qt5sdk/exports section.

### Version number not updating in About dialog
The version is compiled into `qzcommon.o` from `QUPZILLA_VERSION` macro. After changing `QZ_VERSION` in `defines.pri`, force rebuild:
```bash
touch source/qupzilla/src/lib/app/qzcommon.cpp && cd build-arm && make
```

### Resource files not updating after changes
Qt resource files (.qrc) need explicit rebuild when embedded files change:
```bash
rm -f ../../../source/qupzilla/build/qrc_data.cpp && make
```

### Forcing full rebuild
```bash
cd build-arm && make clean && make -j$(nproc)
```

## CRITICAL: Things That Break the App (DO NOT USE)

The following have been tested and cause crashes or hangs. **NEVER use these**:

1. **`--single-process` in QTWEBENGINE_CHROMIUM_FLAGS**: Causes the app to hang indefinitely after entering the event loop. The Qt event loop cannot process timer events when Chromium runs in single-process mode.

2. **Aggressive cleanup of QtWebEngine profile directory before Qt init**: Removing the entire QtWebEngine directory before Qt initialization can cause crashes even on first launch.

3. **Killing processes with `pgrep -f` patterns**: The pattern matching is too broad and can match unrelated processes or cause false positives.

4. **Calling `setTabOrder()` during early initialization**: Qt's `setTabOrder()` function crashes if called before widgets are fully initialized. This particularly affects deferred/async code paths. If you need to call functions that include `setTabOrder()` from a deferred context, either skip the tab order setup or ensure all widgets are fully ready.

5. **Using `palm-launch -c` to kill PDK apps**: This command does not work for PDK apps like QupZilla. Use `echo "killall qupzilla" | novacom run file://bin/sh` instead.

6. **GPU compositing flags that hide the UI**: The following Chromium flags cause the Qt widget layer (toolbar, navigation bar) to disappear behind the web content. **NEVER use these**:
   - `--enable-zero-copy` - Breaks layer compositing
   - `--disable-gpu-vsync` - Causes rendering issues
   - `--ignore-gpu-blocklist` - Forces unsupported GPU features
   - `--enable-accelerated-2d-canvas` - Affects compositing
   - `QSG_RENDER_LOOP=basic` - Breaks Qt widget rendering

   These flags affect how Chromium composites layers with Qt widgets. The PowerVR SGX540 doesn't properly support the layer ordering these flags require.

## Future: Web Engine Upgrade

QtWebEngine in Qt 5.9 is based on Chromium ~56. Upgrading options:
- Qt 5.12 LTS (Chromium 69) or Qt 5.15 LTS (Chromium 83) - would require rebuilding entire Qt
- WebOS hardware (SGX540 GPU) limits OpenGL ES 2.0 support which constrains newer Chromium versions

## GPU Acceleration (Implemented)

The HP TouchPad's PowerVR SGX540 GPU with OpenGL ES 2.0 is now used for hardware-accelerated rendering.

### Working Configuration

```cpp
qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
    "--use-gl=egl "                         // Use EGL for OpenGL ES
    "--enable-gpu-rasterization "           // GPU-accelerated page painting
    "--enable-native-gpu-memory-buffers "   // Efficient GPU memory transfers
    "--num-raster-threads=2 "               // Parallel rasterization on dual-core CPU
    "--disable-background-timer-throttling" // Keep JS timers responsive
);
```

### What Works
- **EGL-based OpenGL ES** - `--use-gl=egl` enables hardware rendering
- **GPU rasterization** - Pages are painted using the GPU
- **Native GPU memory buffers** - Efficient texture/buffer management
- **Parallel rasterization** - Uses both CPU cores for raster work
- **Responsive timers** - Background throttling disabled for better interactivity

### What Breaks the UI (DO NOT USE)
These flags cause the toolbar and navigation bar to disappear (rendered behind web content):
- `--enable-zero-copy` - Breaks layer compositing order
- `--disable-gpu-vsync` - Causes rendering/compositing issues
- `--ignore-gpu-blocklist` - Forces unsupported GPU features
- `--enable-accelerated-2d-canvas` - Affects layer compositing
- `--disable-gpu-compositing` - Does not fix the issue when combined with other GPU flags
- `QSG_RENDER_LOOP=basic` - Breaks Qt widget layer rendering

### Fallback to Software Rendering
If GPU issues occur, use this software-only configuration:
```cpp
qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
    "--disable-gpu "
    "--disable-gpu-compositing "
    "--enable-software-rasterizer");
qputenv("QMLSCENE_DEVICE", "softwarecontext");
qputenv("QT_QUICK_BACKEND", "software");
```

## Unattended Device Iteration

When debugging crashes or issues that require repeated build-test cycles on the webOS device, use this workflow to iterate without user intervention:

### Key Commands

| Command | Purpose |
|---------|---------|
| `palm-package com.nizovn.qupzilla` | Create IPK package from staging directory |
| `palm-install <ipk>` | Install package on device (auto-packages if given directory) |
| `palm-launch com.nizovn.qupzilla` | Launch app on device (non-blocking, returns immediately) |
| `echo "killall qupzilla" \| novacom run file://bin/sh` | Kill/close app on device (for PDK apps) |
| `novacom get file://media/internal/qupzilla/startup.log` | Pull log file from device |

### Important: Killing PDK Apps

**`palm-launch -c` does NOT work for PDK apps like QupZilla.** You must use `killall` via novacom:

```bash
# Kill the app (this is the ONLY reliable way for PDK apps)
echo "killall qupzilla" | novacom run file://bin/sh

# Wait for process to fully terminate before relaunching
sleep 15
```

The `novacom run file://bin/sh` command will output "qupzilla: no process killed" if the app wasn't running, which is normal.

### Important Distinctions

- **`palm-launch`** (preferred): Launches app and returns immediately. App continues running independently. Use this for iteration.
- **`palm-run`** (avoid for iteration): Launches app and streams stdout/stderr. Killing palm-run kills the app. Harder to script.

### Iteration Workflow

```bash
# 1. Build (from build-arm directory, or use make -C)
cd /path/to/build-arm
touch /path/to/modified/file.cpp  # Force rebuild if needed
make -j4

# 2. Copy binaries to staging
cd /path/to/package-staging
cp ../source/qupzilla/bin/qupzilla com.nizovn.qupzilla/bin/
cp ../source/qupzilla/bin/libQupZilla.so.2.3.0 com.nizovn.qupzilla/bin/
cd com.nizovn.qupzilla/bin
rm -f libQupZilla.so libQupZilla.so.2
ln -s libQupZilla.so.2.3.0 libQupZilla.so
ln -s libQupZilla.so.2.3.0 libQupZilla.so.2

# 3. Package and install
cd ../..
palm-package com.nizovn.qupzilla
palm-install com.nizovn.qupzilla_2.3.0_all.ipk

# 4. Kill any existing instance, launch, and wait for startup
echo "killall qupzilla" | novacom run file://bin/sh
sleep 10
palm-launch com.nizovn.qupzilla
sleep 40  # Wait for app to start or crash

# 5. Pull logs and analyze
novacom get file://media/internal/qupzilla/startup.log

# 6. If testing relaunches, kill and relaunch
echo "killall qupzilla" | novacom run file://bin/sh
sleep 15
palm-launch com.nizovn.qupzilla
sleep 35
novacom get file://media/internal/qupzilla/startup.log | tail -15
```

### Verification Testing

When a fix appears to work, verify with multiple consecutive relaunches:

```bash
for i in 1 2 3 4 5; do
  echo "=== RELAUNCH TEST $i ==="
  echo "killall qupzilla" | novacom run file://bin/sh
  sleep 15
  palm-launch com.nizovn.qupzilla
  sleep 35
  novacom get file://media/internal/qupzilla/startup.log | tail -15
done
```

### Log File Location

The app writes startup logs to `/media/internal/qupzilla/startup.log` on the device. This file is overwritten on each launch, so pull it before relaunching if you need to preserve it.

### Timing Considerations

- **Initial startup**: Wait 45 seconds - QtWebEngine/Chromium initialization is slow on this hardware
- **Relaunches**: Wait 35 seconds - stale process cleanup adds overhead
- **Between close and relaunch**: Wait 15 seconds - allow process to fully terminate
- **Time-dependent crashes**: Some crashes (like Chromium IPC errors) are timing-sensitive. If crashes are intermittent, the root cause may be race conditions during initialization

### Shell Syntax Note

When using `make -j$(nproc)` in scripts, the `$()` syntax may cause issues. Use a fixed number like `make -j4` instead for reliability.
