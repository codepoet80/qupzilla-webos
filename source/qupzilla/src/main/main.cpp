/* ============================================================
* QupZilla - Qt web browser
* Copyright (C) 2010-2017 David Rosca <nowrep@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#include "mainapplication.h"
#include "proxystyle.h"
#include "datapaths.h"

#include <QMessageBox> // For QT_REQUIRE_VERSION
#include <iostream>

#if defined(PORTABLE_BUILD) && defined(Q_OS_LINUX)
#include <QSslConfiguration>
#include <QSslCertificate>
#include <QString>
#endif

#if defined(Q_OS_LINUX) || defined(__GLIBC__) || defined(__FreeBSD__) || defined(__HAIKU__)
#include <signal.h>
#include <execinfo.h>
#include <dirent.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/time.h>

#include <QDir>
#include <QDateTime>
#include <QTextStream>
#include <QWebEnginePage>

// Startup logging - enabled by creating /media/internal/qupzilla/enable_logging file
static FILE* g_logFile = nullptr;
static bool g_loggingEnabled = false;
static struct timeval g_startTime;

// Check if logging should be enabled (file-based flag)
static bool shouldEnableLogging()
{
    struct stat st;
    return (stat("/media/internal/qupzilla/enable_logging", &st) == 0);
}

// Get milliseconds since startup for logging
static long getElapsedMs()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec - g_startTime.tv_sec) * 1000 +
           (now.tv_usec - g_startTime.tv_usec) / 1000;
}

static void logMsg(const char* msg)
{
    if (!g_loggingEnabled) return;
    char buf[1024];
    snprintf(buf, sizeof(buf), "[%6ldms] %s", getElapsedMs(), msg);
    std::cerr << buf << std::endl;
    if (g_logFile) {
        fprintf(g_logFile, "%s\n", buf);
        fflush(g_logFile);
    }
}

static void logMsgFmt(const char* fmt, ...)
{
    if (!g_loggingEnabled) return;
    char msg[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);
    logMsg(msg);
}

// Simple cleanup of /tmp IPC files
static void cleanupStaleChromiumFiles()
{
    const char* tmpdir = "/tmp";
    DIR* dir = opendir(tmpdir);
    if (dir) {
        struct dirent* entry;
        char filepath[512];

        while ((entry = readdir(dir)) != NULL) {
            // Clean up Chromium/Mojo IPC files and QtSingleApp lock files
            bool shouldRemove = false;
            if (strncmp(entry->d_name, ".org.chromium.", 14) == 0 ||
                strncmp(entry->d_name, ".com.google.Chrome", 18) == 0) {
                shouldRemove = true;
            }
            if (strncasecmp(entry->d_name, "qtsingleapp-qupzil", 18) == 0) {
                shouldRemove = true;
            }

            if (shouldRemove) {
                snprintf(filepath, sizeof(filepath), "%s/%s", tmpdir, entry->d_name);
                unlink(filepath);
            }
        }
        closedir(dir);
    }
}

void qupzilla_signal_handler(int s)
{
    if (s != SIGSEGV) {
        return;
    }

    static bool sigSegvServed = false;
    if (sigSegvServed) {
        abort();
    }
    sigSegvServed = true;

    std::cout << "QupZilla: Crashed :( Saving backtrace in " << qPrintable(DataPaths::path(DataPaths::Config)) << "/crashlog ..." << std::endl;

    void* array[100];
    int size = backtrace(array, 100);
    char** strings = backtrace_symbols(array, size);

    if (size < 0 || !strings) {
        std::cout << "Cannot get backtrace!" << std::endl;
        abort();
    }

    QDir dir(DataPaths::path(DataPaths::Config));
    if (!dir.exists()) {
        std::cout << qPrintable(DataPaths::path(DataPaths::Config)) << " does not exist" << std::endl;
        abort();
    }

    if (!dir.cd("crashlog")) {
        if (!dir.mkdir("crashlog")) {
            std::cout << "Cannot create " << qPrintable(DataPaths::path(DataPaths::Config)) << "crashlog directory!" << std::endl;
            abort();
        }

        dir.cd("crashlog");
    }

    const QDateTime currentDateTime = QDateTime::currentDateTime();

    QFile file(dir.absoluteFilePath("Crash-" + currentDateTime.toString(Qt::ISODate) + ".txt"));
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        std::cout << "Cannot open file " << qPrintable(file.fileName()) << " for writing!" << std::endl;
        abort();
    }

    QTextStream stream(&file);
    stream << "Time: " << currentDateTime.toString() << endl;
    stream << "Qt version: " << qVersion() << " (compiled with " << QT_VERSION_STR << ")" << endl;
    stream << "QupZilla version: " << Qz::VERSION << endl;
    stream << "Rendering engine: QtWebEngine" << endl;
    stream << endl;
    stream << "============== BACKTRACE ==============" << endl;

    for (int i = 0; i < size; ++i) {
        stream << "#" << i << ": " << strings[i] << endl;
    }

    file.close();

    std::cout << "Backtrace successfully saved in " << qPrintable(dir.absoluteFilePath(file.fileName())) << std::endl;
}
#endif // defined(Q_OS_LINUX) || defined(__GLIBC__) || defined(__FreeBSD__)

#ifndef Q_OS_WIN
void msgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (msg.startsWith(QL1S("QSslSocket: cannot resolve SSL")))
        return;
    if (msg.startsWith(QL1S("Remote debugging server started successfully.")))
        return;

    switch (type) {
    case QtDebugMsg:
    case QtInfoMsg:
    case QtWarningMsg:
    case QtCriticalMsg:
        std::cerr << qPrintable(qFormatLogMessage(type, context, msg)) << std::endl;
        break;

    case QtFatalMsg:
        std::cerr << "Fatal: " << qPrintable(qFormatLogMessage(type, context, msg)) << std::endl;
        abort();

    default:
        break;
    }
}
#endif

int main(int argc, char* argv[])
{
    gettimeofday(&g_startTime, NULL);

#if defined(Q_OS_LINUX) || defined(__GLIBC__) || defined(__FreeBSD__)
    // Check for logging flag file
    g_loggingEnabled = shouldEnableLogging();
    if (g_loggingEnabled) {
        mkdir("/media/internal/qupzilla", 0755);
        g_logFile = fopen("/media/internal/qupzilla/startup.log", "w");
        logMsg("[STARTUP] QupZilla starting (logging enabled via flag file)");
    }

    // Clean up stale IPC files
    cleanupStaleChromiumFiles();
#endif

    QT_REQUIRE_VERSION(argc, argv, "5.8.0");

#ifndef Q_OS_WIN
    qInstallMessageHandler(&msgHandler);
#endif

#if defined(Q_OS_LINUX) || defined(__GLIBC__) || defined(__FreeBSD__)
    signal(SIGSEGV, qupzilla_signal_handler);
#endif

#if defined(PORTABLE_BUILD) && defined(Q_OS_LINUX)
    logMsg("[STARTUP] Setting webOS environment variables");
    // webOS-specific environment setup - must be set before QApplication
    // GPU acceleration with safe performance flags
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
        "--use-gl=egl "
        "--enable-gpu-rasterization "
        "--enable-native-gpu-memory-buffers "
        "--num-raster-threads=2 "
        "--disable-background-timer-throttling");
    qputenv("QT_QPA_FONTDIR", "/usr/share/fonts");
    qputenv("QT_QPA_WEBOS_PHYSICAL_WIDTH", "197");
    qputenv("QT_QPA_WEBOS_PHYSICAL_HEIGHT", "148");
    qputenv("SSL_CERT_FILE", "/media/cryptofs/apps/usr/palm/applications/com.nizovn.cacert/files/cert.pem");
#endif

    // Hack to fix QT_STYLE_OVERRIDE with QProxyStyle
    const QByteArray style = qgetenv("QT_STYLE_OVERRIDE");
    if (!style.isEmpty()) {
        char** args = (char**) malloc(sizeof(char*) * (argc + 1));
        for (int i = 0; i < argc; ++i)
            args[i] = argv[i];

        QString stylecmd = QL1S("-style=") + style;
        args[argc++] = qstrdup(stylecmd.toUtf8().constData());
        argv = args;
    }

#if defined(PORTABLE_BUILD) && defined(Q_OS_LINUX)
    // Log all argv for debugging
    logMsgFmt("[STARTUP] argc = %d", argc);
    for (int i = 0; i < argc; ++i) {
        logMsgFmt("[STARTUP] argv[%d] = %s", i, argv[i]);
    }

    // Parse launch URL from command-line arguments
    // webOS passes params as JSON array string: [ "http:\/\/example.com" ]
    QString launchUrl;
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        // Skip Qt-style arguments
        if (arg.startsWith(QLatin1String("-"))) {
            continue;
        }

        // Check if argument is a JSON array (webOS format)
        // Note: URLs in JSON have escaped slashes (\/) so check for that
        if (arg.startsWith(QLatin1String("[")) && arg.contains(QLatin1String("\\/"))) {
            // Extract URL from JSON array: [ "http:\/\/example.com" ]
            // Find the quoted string inside
            int firstQuote = arg.indexOf('"');
            int lastQuote = arg.lastIndexOf('"');
            if (firstQuote != -1 && lastQuote > firstQuote) {
                QString url = arg.mid(firstQuote + 1, lastQuote - firstQuote - 1);
                // Unescape JSON slashes
                url.replace(QLatin1String("\\/"), QLatin1String("/"));
                launchUrl = url;
                logMsgFmt("[STARTUP] Parsed URL from JSON: %s", launchUrl.toUtf8().constData());
            }
            break;
        }

        // Check if argument is a direct URL
        if (arg.startsWith(QLatin1String("http://")) ||
            arg.startsWith(QLatin1String("https://")) ||
            arg.startsWith(QLatin1String("file://")) ||
            arg.contains(QLatin1String("://"))) {
            launchUrl = arg;
            logMsgFmt("[STARTUP] Found direct URL in argv: %s", arg.toUtf8().constData());
            break;
        }
    }

    // Cache URL for opening after window is fully ready
    if (!launchUrl.isEmpty()) {
        logMsgFmt("[STARTUP] Setting pending launch URL: %s", launchUrl.toUtf8().constData());
        MainApplication::setPendingLaunchUrl(launchUrl);
    }
#endif

    logMsg("[STARTUP] Creating MainApplication");
    MainApplication app(argc, argv);

    if (app.isClosing()) {
        if (g_logFile) fclose(g_logFile);
        return 0;
    }

#if defined(PORTABLE_BUILD) && defined(Q_OS_LINUX)
    // Load SSL certificates
    QList<QSslCertificate> systemCerts = QSslConfiguration::systemCaCertificates();
    if (!systemCerts.isEmpty()) {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setCaCertificates(systemCerts);
        QSslConfiguration::setDefaultConfiguration(sslConfig);
        logMsgFmt("[STARTUP] SSL certificates loaded: %d certs", systemCerts.size());
    }
#endif

    app.setProxyStyle(new ProxyStyle);
    app.processEvents();

    logMsg("[STARTUP] Entering event loop");
    int result = app.exec();

    logMsgFmt("[SHUTDOWN] Exit code %d", result);
    if (g_logFile) fclose(g_logFile);
    return result;
}
