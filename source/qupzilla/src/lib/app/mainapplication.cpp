/* ============================================================
* QupZilla - Qt web browser
* Copyright (C) 2010-2018 David Rosca <nowrep@gmail.com>
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
#include "history.h"
#include <iostream>
#include <cstdio>
#include <sys/stat.h>

// Check if logging is enabled via flag file
static bool isLoggingEnabled() {
    struct stat st;
    return (stat("/media/internal/qupzilla/enable_logging", &st) == 0);
}

// Helper to log to both stderr and file (only if logging enabled)
static void appLog(const char* msg) {
    if (!isLoggingEnabled()) return;
    std::cerr << msg << std::endl;
    FILE* f = fopen("/media/internal/qupzilla/startup.log", "a");
    if (f) {
        fprintf(f, "%s\n", msg);
        fclose(f);
    }
}
#include "qztools.h"
#include "updater.h"
#include "autofill.h"
#include "settings.h"
#include "qzregexp.h"
#include "autosaver.h"
#include "datapaths.h"
#include "tabwidget.h"
#include "cookiejar.h"
#include "bookmarks.h"
#include "qzsettings.h"
#include "proxystyle.h"
#include "pluginproxy.h"
#include "iconprovider.h"
#include "browserwindow.h"
#include "checkboxdialog.h"
#include "networkmanager.h"
#include "profilemanager.h"
#include "restoremanager.h"
#include "browsinglibrary.h"
#include "downloadmanager.h"
#include "clearprivatedata.h"
#include "useragentmanager.h"
#include "commandlineoptions.h"
#include "searchenginesmanager.h"
#include "desktopnotificationsfactory.h"
#include "html5permissions/html5permissionsmanager.h"
#include "scripts.h"
#include "sessionmanager.h"
#include "closedwindowsmanager.h"

#include <QWebEngineSettings>
#include <QDesktopServices>
#include <QFontDatabase>
#include <QSqlDatabase>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QTranslator>
#include <QThreadPool>
#include <QThread>
#include <QSettings>
#include <QProcess>
#include <QTimer>
#include <QDir>
#include <QStandardPaths>
#include <QWebEngineProfile>
#include <QWebEngineDownloadItem>
#include <QWebEngineScriptCollection>

#include <iostream>

static bool s_testMode = false;
QString MainApplication::s_pendingLaunchUrl;

MainApplication::MainApplication(int &argc, char** argv)
    : QtSingleApplication(argc, argv)
    , m_isPrivate(false)
    , m_isPortable(false)
    , m_isClosing(false)
    , m_isStartingAfterCrash(false)
    , m_history(0)
    , m_bookmarks(0)
    , m_autoFill(0)
    , m_cookieJar(0)
    , m_plugins(0)
    , m_browsingLibrary(0)
    , m_networkManager(0)
    , m_restoreManager(0)
    , m_sessionManager(0)
    , m_downloadManager(0)
    , m_userAgentManager(0)
    , m_searchEnginesManager(0)
    , m_closedWindowsManager(0)
    , m_html5PermissionsManager(0)
    , m_desktopNotifications(0)
    , m_webProfile(0)
    , m_autoSaver(0)
{
    std::cerr << "[MAINAPPLICATION] Constructor started" << std::endl;
    setAttribute(Qt::AA_UseHighDpiPixmaps);
    setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    std::cerr << "[MAINAPPLICATION] Setting application name and icon" << std::endl;
    setApplicationName(QLatin1String("QupZilla"));
    setOrganizationDomain(QLatin1String("qupzilla"));
    setWindowIcon(QIcon::fromTheme(QSL("qupzilla"), QIcon(QSL(":icons/exeicons/qupzilla-window.png"))));
    setDesktopFileName(QSL("org.qupzilla.QupZilla"));

#ifdef GIT_REVISION
    setApplicationVersion(QSL("%1 (%2)").arg(Qz::VERSION, GIT_REVISION));
#else
    setApplicationVersion(Qz::VERSION);
#endif

    // Set fallback icon theme (eg. on Windows/Mac)
    if (QIcon::fromTheme(QSL("view-refresh")).isNull()) {
        QIcon::setThemeName(QSL("breeze-fallback"));
    }

    // QSQLITE database plugin is required
    std::cerr << "[MAINAPPLICATION] Checking SQLite driver" << std::endl;
    if (!QSqlDatabase::isDriverAvailable(QSL("QSQLITE"))) {
        QMessageBox::critical(0, QSL("Error"), QSL("Qt SQLite database plugin is not available. Please install it and restart the application."));
        m_isClosing = true;
        return;
    }
    std::cerr << "[MAINAPPLICATION] SQLite driver OK" << std::endl;

    QUrl startUrl;
    QString startProfile;
    QStringList messages;

    bool noAddons = false;
    bool newInstance = false;

    if (argc > 1) {
        CommandLineOptions cmd;
        foreach (const CommandLineOptions::ActionPair &pair, cmd.getActions()) {
            switch (pair.action) {
            case Qz::CL_StartWithoutAddons:
                noAddons = true;
                break;
            case Qz::CL_StartWithProfile:
                startProfile = pair.text;
                break;
            case Qz::CL_StartPortable:
                m_isPortable = true;
                break;
            case Qz::CL_NewTab:
                messages.append(QLatin1String("ACTION:NewTab"));
                m_postLaunchActions.append(OpenNewTab);
                break;
            case Qz::CL_NewWindow:
                messages.append(QLatin1String("ACTION:NewWindow"));
                break;
            case Qz::CL_ToggleFullScreen:
                messages.append(QLatin1String("ACTION:ToggleFullScreen"));
                m_postLaunchActions.append(ToggleFullScreen);
                break;
            case Qz::CL_ShowDownloadManager:
                messages.append(QLatin1String("ACTION:ShowDownloadManager"));
                m_postLaunchActions.append(OpenDownloadManager);
                break;
            case Qz::CL_StartPrivateBrowsing:
                m_isPrivate = true;
                break;
            case Qz::CL_StartNewInstance:
                newInstance = true;
                break;
            case Qz::CL_OpenUrlInCurrentTab:
                startUrl = QUrl::fromUserInput(pair.text);
                messages.append("ACTION:OpenUrlInCurrentTab" + pair.text);
                break;
            case Qz::CL_OpenUrlInNewWindow:
                startUrl = QUrl::fromUserInput(pair.text);
                messages.append("ACTION:OpenUrlInNewWindow" + pair.text);
                break;
            case Qz::CL_OpenUrl:
                startUrl = QUrl::fromUserInput(pair.text);
                messages.append("URL:" + pair.text);
                break;
            case Qz::CL_ExitAction:
                m_isClosing = true;
                return;
            default:
                break;
            }
        }
    }

    std::cerr << "[MAINAPPLICATION] Checking portable mode" << std::endl;
    if (isPortable()) {
        std::cout << "QupZilla: Running in Portable Mode." << std::endl;
        std::cerr << "[MAINAPPLICATION] Setting portable version paths" << std::endl;
        DataPaths::setPortableVersion();
    }

    // Don't start single application in private browsing
    if (!isPrivate()) {
        QString appId = QLatin1String("QupZillaWebBrowser");

        if (isPortable()) {
            appId.append(QLatin1String("Portable"));
        }

        if (isTestModeEnabled()) {
            appId.append(QSL("TestMode"));
        }

        if (newInstance) {
            if (startProfile.isEmpty() || startProfile == QLatin1String("default")) {
                std::cout << "New instance cannot be started with default profile!" << std::endl;
            }
            else {
                // Generate unique appId so it is possible to start more separate instances
                // of the same profile. It is dangerous to run more instances of the same profile,
                // but if the user wants it, we should allow it.
                appId.append(startProfile + QString::number(QDateTime::currentMSecsSinceEpoch()));
            }
        }

        setAppId(appId);
    }

    // If there is nothing to tell other instance, we need to at least wake it
    if (messages.isEmpty()) {
        messages.append(QLatin1String(" "));
    }

    std::cerr << "[MAINAPPLICATION] Checking if already running" << std::endl;
    if (isRunning()) {
        std::cerr << "[MAINAPPLICATION] Already running, sending messages and closing" << std::endl;
        m_isClosing = true;
        foreach (const QString &message, messages) {
            sendMessage(message);
        }
        return;
    }
    std::cerr << "[MAINAPPLICATION] Not already running, continuing startup" << std::endl;

    setQuitOnLastWindowClosed(true);

    std::cerr << "[MAINAPPLICATION] Setting up URL handlers" << std::endl;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QDesktopServices::setUrlHandler(QSL("http"), this, "addNewTab");
    QDesktopServices::setUrlHandler(QSL("https"), this, "addNewTab");
    QDesktopServices::setUrlHandler(QSL("ftp"), this, "addNewTab");

    std::cerr << "[MAINAPPLICATION] Initializing ProfileManager" << std::endl;
    ProfileManager profileManager;
    profileManager.initConfigDir();
    std::cerr << "[MAINAPPLICATION] initConfigDir done" << std::endl;
    profileManager.initCurrentProfile(startProfile);
    std::cerr << "[MAINAPPLICATION] initCurrentProfile done" << std::endl;

    std::cerr << "[MAINAPPLICATION] Creating Settings" << std::endl;
    Settings::createSettings(DataPaths::currentProfilePath() + QLatin1String("/settings.ini"));

#ifdef PORTABLE_BUILD
    // Clean up QtWebEngine directories that might have stale state causing IPC crashes
    appLog("[MAINAPPLICATION] Cleaning QtWebEngine directories");
    QString profilePath = DataPaths::currentProfilePath();
    appLog(qPrintable(QSL("[MAINAPPLICATION] Profile path: ") + profilePath));
    QDir webEngineDir(profilePath + "/QtWebEngine");
    if (webEngineDir.exists()) {
        appLog(qPrintable(QSL("[MAINAPPLICATION] Removing QtWebEngine dir: ") + webEngineDir.absolutePath()));
        webEngineDir.removeRecursively();
    }
    QDir webEngineDefaultDir(profilePath + "/QtWebEngine/Default");
    if (webEngineDefaultDir.exists()) {
        appLog("[MAINAPPLICATION] Removing QtWebEngine/Default dir");
        webEngineDefaultDir.removeRecursively();
    }
    // Also check for cache directories
    QDir cacheDir(DataPaths::path(DataPaths::Cache) + "/QtWebEngine");
    if (cacheDir.exists()) {
        appLog(qPrintable(QSL("[MAINAPPLICATION] Removing cache QtWebEngine dir: ") + cacheDir.absolutePath()));
        cacheDir.removeRecursively();
    }
#endif

#ifdef PORTABLE_BUILD
    // PORTABLE_BUILD: Defer QWebEngineProfile creation until event loop is running
    // Creating it before event loop causes Chromium IPC channel corruption on relaunch
    appLog("[MAINAPPLICATION] PORTABLE_BUILD: Deferring QWebEngineProfile creation to event loop");
    m_webProfile = nullptr;  // Will be created after event loop starts
#else
    appLog("[MAINAPPLICATION] Creating QWebEngineProfile");
    m_webProfile = isPrivate() ? new QWebEngineProfile(this) : QWebEngineProfile::defaultProfile();
    appLog("[MAINAPPLICATION] QWebEngineProfile created");
    connect(m_webProfile, &QWebEngineProfile::downloadRequested, this, &MainApplication::downloadRequested);
#endif

#ifndef PORTABLE_BUILD
    appLog("[MAINAPPLICATION] Creating NetworkManager");
    m_networkManager = new NetworkManager(this);
#else
    appLog("[MAINAPPLICATION] PORTABLE_BUILD: Deferring NetworkManager (needs webProfile)");
#endif

    // Force IconProvider initialization in main thread to prevent QPixmap threading issues
    // when LocationCompleterRefreshJob or other background threads access it first
    appLog("[MAINAPPLICATION] Initializing IconProvider");
    IconProvider::instance();

#ifndef PORTABLE_BUILD
    appLog("[MAINAPPLICATION] Setting up user scripts");
    setupUserScripts();
#endif

#ifndef PORTABLE_BUILD
    if (!isPrivate() && !isTestModeEnabled()) {
        m_sessionManager = new SessionManager(this);
        m_autoSaver = new AutoSaver(this);
        connect(m_autoSaver, SIGNAL(save()), m_sessionManager, SLOT(autoSaveLastSession()));

        Settings settings;
        settings.beginGroup(QSL("SessionRestore"));
        const bool wasRunning = settings.value(QSL("isRunning"), false).toBool();
        const bool wasRestoring = settings.value(QSL("isRestoring"), false).toBool();
        settings.setValue(QSL("isRunning"), true);
        settings.setValue(QSL("isRestoring"), wasRunning);
        settings.endGroup();
        settings.sync();

        m_isStartingAfterCrash = wasRunning && wasRestoring;

        if (wasRunning) {
            QTimer::singleShot(60 * 1000, this, [this]() {
                Settings().setValue(QSL("SessionRestore/isRestoring"), false);
            });
        }

        // we have to ask about startup session before creating main window
        if (!m_isStartingAfterCrash && afterLaunch() == SelectSession)
            m_restoreManager = new RestoreManager(sessionManager()->askSessionFromUser());
    }
#endif

    appLog("[MAINAPPLICATION] Translating app");
    translateApp();
#ifndef PORTABLE_BUILD
    appLog("[MAINAPPLICATION] Loading settings");
    loadSettings();
    appLog("[MAINAPPLICATION] Settings loaded");
#else
    appLog("[MAINAPPLICATION] PORTABLE_BUILD: Deferring loadSettings (needs webProfile)");
#endif

    appLog("[MAINAPPLICATION] Creating PluginProxy");
    m_plugins = new PluginProxy(this);
#ifndef PORTABLE_BUILD
    appLog("[MAINAPPLICATION] Creating AutoFill");
    m_autoFill = new AutoFill(this);
#else
    appLog("[MAINAPPLICATION] PORTABLE_BUILD: Deferring AutoFill (needs webProfile)");
#endif

#ifndef PORTABLE_BUILD
    if (!noAddons) {
        appLog("[MAINAPPLICATION] Loading plugins");
        m_plugins->loadPlugins();
        appLog("[MAINAPPLICATION] Plugins loaded");
    }
#else
    appLog("[MAINAPPLICATION] PORTABLE_BUILD: Deferring loadPlugins (needs networkManager)");
    Q_UNUSED(noAddons);
#endif

    appLog("[MAINAPPLICATION] Creating browser window");
#ifdef PORTABLE_BUILD
    // Defer ALL WebEngine initialization until event loop is running to avoid Chromium IPC crashes
    // Creating QWebEngineProfile before event loop causes "Invalid node channel message" on relaunch
    appLog("[MAINAPPLICATION] PORTABLE_BUILD: Deferring WebEngine init");
    // Stage 1: Create QWebEngineProfile (starts Chromium subprocess)
    // Minimal delay - just let event loop start
    QTimer::singleShot(100, this, [this]() {
        appLog("[MAINAPPLICATION] Stage1: Creating QWebEngineProfile");
        m_webProfile = new QWebEngineProfile(this);
        connect(m_webProfile, &QWebEngineProfile::downloadRequested, this, &MainApplication::downloadRequested);

        // Stage 2: Brief wait for Chromium to initialize
        QTimer::singleShot(500, this, [this]() {
            appLog("[MAINAPPLICATION] Stage2: Initializing components");

            m_networkManager = new NetworkManager(this);
            m_autoFill = new AutoFill(this);
            m_plugins->loadPlugins();
            loadSettings();
            setupUserScripts();

            appLog("[MAINAPPLICATION] Stage2: Creating browser window");
            BrowserWindow* window = createWindow(Qz::BW_FirstAppWindow);
            connect(window, SIGNAL(startingCompleted()), this, SLOT(restoreOverrideCursor()));
            connect(window, SIGNAL(startingCompleted()), this, SLOT(onWindowStartingCompleted()));
        });
    });
#else
    BrowserWindow* window = createWindow(Qz::BW_FirstAppWindow, startUrl);
    appLog("[MAINAPPLICATION] Browser window created");
    connect(window, SIGNAL(startingCompleted()), this, SLOT(restoreOverrideCursor()));
#endif

    connect(this, SIGNAL(focusChanged(QWidget*,QWidget*)), this, SLOT(onFocusChanged()));

    if (!isPrivate() && !isTestModeEnabled()) {
#ifndef DISABLE_CHECK_UPDATES
        Settings settings;
        bool checkUpdates = settings.value("Web-Browser-Settings/CheckUpdates", true).toBool();

        if (checkUpdates) {
            new Updater(window);
        }
#endif

#ifndef PORTABLE_BUILD
        sessionManager()->backupSavedSessions();

        if (m_isStartingAfterCrash || afterLaunch() == RestoreSession) {
            m_restoreManager = new RestoreManager(sessionManager()->lastActiveSessionPath());
            if (!m_restoreManager->isValid()) {
                destroyRestoreManager();
            }
        }

        if (!m_isStartingAfterCrash && m_restoreManager) {
            restoreSession(window, m_restoreManager->restoreData());
        }
#endif
    }

    appLog("[MAINAPPLICATION] Constructor completed, scheduling postLaunch");
    QTimer::singleShot(0, this, SLOT(postLaunch()));
}

MainApplication::~MainApplication()
{
    IconProvider::instance()->saveIconsToDatabase();

    // Wait for all QtConcurrent jobs to finish
    QThreadPool::globalInstance()->waitForDone();

    // Delete all classes that are saving data in destructor
    delete m_bookmarks;
    m_bookmarks = nullptr;
    delete m_cookieJar;
    m_cookieJar = nullptr;

    Settings::syncSettings();
}

bool MainApplication::isClosing() const
{
    return m_isClosing;
}

bool MainApplication::isPrivate() const
{
    return m_isPrivate;
}

bool MainApplication::isPortable() const
{
#ifdef PORTABLE_BUILD
    return true;
#else
    return m_isPortable;
#endif
}

bool MainApplication::isStartingAfterCrash() const
{
    return m_isStartingAfterCrash;
}

int MainApplication::windowCount() const
{
    return m_windows.count();
}

QList<BrowserWindow*> MainApplication::windows() const
{
    return m_windows;
}

BrowserWindow* MainApplication::getWindow() const
{
    if (m_lastActiveWindow) {
        return m_lastActiveWindow.data();
    }

    return m_windows.isEmpty() ? 0 : m_windows.at(0);
}

BrowserWindow* MainApplication::createWindow(Qz::BrowserWindowType type, const QUrl &startUrl)
{
    if (windowCount() == 0 && type != Qz::BW_MacFirstWindow) {
        type = Qz::BW_FirstAppWindow;
    }

    BrowserWindow* window = new BrowserWindow(type, startUrl);
    connect(window, SIGNAL(destroyed(QObject*)), this, SLOT(windowDestroyed(QObject*)));

    m_windows.prepend(window);
    return window;
}

MainApplication::AfterLaunch MainApplication::afterLaunch() const
{
#ifdef PORTABLE_BUILD
    // Always open Start Page on startup for webOS portable build
    return OpenHomePage;
#else
    return static_cast<AfterLaunch>(Settings().value(QSL("Web-URL-Settings/afterLaunch"), OpenHomePage).toInt());
#endif
}

void MainApplication::openSession(BrowserWindow* window, RestoreData &restoreData)
{
    setOverrideCursor(Qt::BusyCursor);

    if (!window)
        window = createWindow(Qz::BW_OtherRestoredWindow);

    if (window->tabCount() != 0) {
        // This can only happen when recovering crashed session!
        // Don't restore tabs in current window as user already opened some new tabs.
        createWindow(Qz::BW_OtherRestoredWindow)->restoreWindow(restoreData.windows.takeAt(0));
    } else {
        window->restoreWindow(restoreData.windows.takeAt(0));
    }

    foreach (const BrowserWindow::SavedWindow &data, restoreData.windows) {
        BrowserWindow* window = createWindow(Qz::BW_OtherRestoredWindow);
        window->restoreWindow(data);
    }

    m_closedWindowsManager->restoreState(restoreData.closedWindows);

    restoreOverrideCursor();
}

bool MainApplication::restoreSession(BrowserWindow* window, RestoreData restoreData)
{
    if (m_isPrivate || !restoreData.isValid()) {
        return false;
    }

    openSession(window, restoreData);

    m_restoreManager->clearRestoreData();
    destroyRestoreManager();

    return true;
}

void MainApplication::destroyRestoreManager()
{
    if (m_restoreManager && m_restoreManager->isValid()) {
        return;
    }

    delete m_restoreManager;
    m_restoreManager = 0;
}

void MainApplication::reloadSettings()
{
    loadSettings();
    emit settingsReloaded();
}

QString MainApplication::styleName() const
{
    return m_proxyStyle ? m_proxyStyle->name() : QString();
}

void MainApplication::setProxyStyle(ProxyStyle *style)
{
    m_proxyStyle = style;
    setStyle(style);
}

QString MainApplication::currentLanguageFile() const
{
    return m_languageFile;
}

QString MainApplication::currentLanguage() const
{
    QString lang = m_languageFile;

    if (lang.isEmpty()) {
        return "en_US";
    }

    return lang.left(lang.length() - 3);
}

History* MainApplication::history()
{
    if (!m_history) {
        m_history = new History(this);
    }
    return m_history;
}

Bookmarks* MainApplication::bookmarks()
{
    if (!m_bookmarks) {
        m_bookmarks = new Bookmarks(this);
    }
    return m_bookmarks;
}

AutoFill* MainApplication::autoFill()
{
    return m_autoFill;
}

CookieJar* MainApplication::cookieJar()
{
    if (!m_cookieJar) {
        m_cookieJar = new CookieJar(this);
    }
    return m_cookieJar;
}

PluginProxy* MainApplication::plugins()
{
    return m_plugins;
}

BrowsingLibrary* MainApplication::browsingLibrary()
{
    if (!m_browsingLibrary) {
        m_browsingLibrary = new BrowsingLibrary(getWindow());
    }
    return m_browsingLibrary;
}

NetworkManager *MainApplication::networkManager()
{
    return m_networkManager;
}

RestoreManager* MainApplication::restoreManager()
{
    return m_restoreManager;
}

SessionManager* MainApplication::sessionManager()
{
    return m_sessionManager;
}

DownloadManager* MainApplication::downloadManager()
{
    if (!m_downloadManager) {
        m_downloadManager = new DownloadManager();
    }
    return m_downloadManager;
}

UserAgentManager* MainApplication::userAgentManager()
{
    if (!m_userAgentManager) {
        m_userAgentManager = new UserAgentManager(this);
    }
    return m_userAgentManager;
}

SearchEnginesManager* MainApplication::searchEnginesManager()
{
    if (!m_searchEnginesManager) {
        m_searchEnginesManager = new SearchEnginesManager(this);
    }
    return m_searchEnginesManager;
}

ClosedWindowsManager* MainApplication::closedWindowsManager()
{
    if (!m_closedWindowsManager) {
        m_closedWindowsManager = new ClosedWindowsManager(this);
    }
    return m_closedWindowsManager;
}

HTML5PermissionsManager* MainApplication::html5PermissionsManager()
{
    if (!m_html5PermissionsManager) {
        m_html5PermissionsManager = new HTML5PermissionsManager(this);
    }
    return m_html5PermissionsManager;
}

DesktopNotificationsFactory* MainApplication::desktopNotifications()
{
    if (!m_desktopNotifications) {
        m_desktopNotifications = new DesktopNotificationsFactory(this);
    }
    return m_desktopNotifications;
}

QWebEngineProfile *MainApplication::webProfile() const
{
    return m_webProfile;
}

QWebEngineSettings *MainApplication::webSettings() const
{
    return m_webProfile->settings();
}

// static
MainApplication* MainApplication::instance()
{
    return static_cast<MainApplication*>(QCoreApplication::instance());
}

// static
bool MainApplication::isTestModeEnabled()
{
    return s_testMode;
}

// static
void MainApplication::setTestModeEnabled(bool enabled)
{
    s_testMode = enabled;
}

// static
void MainApplication::setPendingLaunchUrl(const QString &url)
{
    s_pendingLaunchUrl = url;
}

// static
QString MainApplication::pendingLaunchUrl()
{
    return s_pendingLaunchUrl;
}

void MainApplication::addNewTab(const QUrl &url)
{
    BrowserWindow* window = getWindow();

    if (window) {
        window->tabWidget()->addView(url, url.isEmpty() ? Qz::NT_SelectedNewEmptyTab : Qz::NT_SelectedTabAtTheEnd);
    }
}

void MainApplication::onWindowStartingCompleted()
{
    // Open pending launch URL if one was provided via command line (webOS PDK launch params)
    if (!s_pendingLaunchUrl.isEmpty()) {
        appLog(QSL("[MAINAPPLICATION] Opening pending launch URL: %1").arg(s_pendingLaunchUrl).toUtf8().constData());
        BrowserWindow* window = getWindow();
        if (window) {
            // Navigate the current tab to the launch URL
            window->loadAddress(QUrl::fromUserInput(s_pendingLaunchUrl));
        }
        s_pendingLaunchUrl.clear();  // Only open once
    }
}

void MainApplication::startPrivateBrowsing(const QUrl &startUrl)
{
    QUrl url = startUrl;
    if (QAction* act = qobject_cast<QAction*>(sender())) {
        url = act->data().toUrl();
    }

    QStringList args;
    args.append(QSL("--private-browsing"));
    args.append(QSL("--profile=") + ProfileManager::currentProfile());

    if (!url.isEmpty()) {
        args << url.toEncoded();
    }

    if (!QProcess::startDetached(applicationFilePath(), args)) {
        qWarning() << "MainApplication: Cannot start new browser process for private browsing!" << applicationFilePath() << args;
    }
}

void MainApplication::reloadUserStyleSheet()
{
    const QString userCssFile = Settings().value("Web-Browser-Settings/userStyleSheet", QString()).toString();
    setUserStyleSheet(userCssFile);
}

void MainApplication::restoreOverrideCursor()
{
    QApplication::restoreOverrideCursor();
}

void MainApplication::changeOccurred()
{
    if (m_autoSaver)
        m_autoSaver->changeOccurred();
}

void MainApplication::quitApplication()
{
    if (m_downloadManager && !m_downloadManager->canClose()) {
        m_downloadManager->show();
        return;
    }

    for (BrowserWindow *window : qAsConst(m_windows)) {
        emit window->aboutToClose();
    }

#ifndef PORTABLE_BUILD
    if (m_sessionManager && m_windows.count() > 0) {
        m_sessionManager->autoSaveLastSession();
    }
#endif

    m_isClosing = true;

    for (BrowserWindow *window : qAsConst(m_windows)) {
        window->close();
    }

    // Saving settings in saveSettings() slot called from quit() so
    // everything gets saved also when quitting application in other
    // way than clicking Quit action in File menu or closing last window
    // eg. on Mac (#157)

    if (!isPrivate()) {
        removeLockFile();
    }

    quit();
}

void MainApplication::postLaunch()
{
    appLog("[MAINAPPLICATION] postLaunch() started");

    if (m_postLaunchActions.contains(OpenDownloadManager)) {
        downloadManager()->show();
    }

    if (m_postLaunchActions.contains(OpenNewTab)) {
        getWindow()->tabWidget()->addView(QUrl(), Qz::NT_SelectedNewEmptyTab);
    }

    if (m_postLaunchActions.contains(ToggleFullScreen)) {
        getWindow()->toggleFullScreen();
    }

    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, DataPaths::currentProfilePath());

    connect(this, SIGNAL(messageReceived(QString)), this, SLOT(messageReceived(QString)));
    connect(this, SIGNAL(aboutToQuit()), this, SLOT(saveSettings()));

    appLog("[MAINAPPLICATION] postLaunch() creating jump list");
    createJumpList();
    appLog("[MAINAPPLICATION] postLaunch() init pulse support");
    initPulseSupport();

    appLog("[MAINAPPLICATION] postLaunch() completed");
    QTimer::singleShot(5000, this, &MainApplication::runDeferredPostLaunchActions);
}

QByteArray MainApplication::saveState() const
{
    RestoreData restoreData;
    restoreData.windows.reserve(m_windows.count());
    for (BrowserWindow *window : qAsConst(m_windows)) {
        restoreData.windows.append(BrowserWindow::SavedWindow(window));
    }

    if (m_restoreManager && m_restoreManager->isValid()) {
        QDataStream stream(&restoreData.crashedSession, QIODevice::WriteOnly);
        stream << m_restoreManager->restoreData();
    }

    restoreData.closedWindows = m_closedWindowsManager->saveState();

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    stream << Qz::sessionVersion;
    stream << restoreData;

    return data;
}

void MainApplication::saveSettings()
{
    if (isPrivate()) {
        return;
    }

    m_isClosing = true;

    Settings settings;
    settings.beginGroup("SessionRestore");
    settings.setValue("isRunning", false);
    settings.setValue("isRestoring", false);
    settings.endGroup();

    settings.beginGroup("Web-Browser-Settings");
    bool deleteCache = settings.value("deleteCacheOnClose", false).toBool();
    bool deleteHistory = settings.value("deleteHistoryOnClose", false).toBool();
    bool deleteHtml5Storage = settings.value("deleteHTML5StorageOnClose", false).toBool();
    settings.endGroup();

    settings.beginGroup("Cookie-Settings");
    bool deleteCookies = settings.value("deleteCookiesOnClose", false).toBool();
    settings.endGroup();

    if (deleteHistory) {
        m_history->clearHistory();
    }
    if (deleteHtml5Storage) {
        ClearPrivateData::clearLocalStorage();
    }
    if (deleteCookies) {
        m_cookieJar->deleteAllCookies();
    }
    if (deleteCache) {
        QzTools::removeDir(mApp->webProfile()->cachePath());
    }

    m_searchEnginesManager->saveSettings();
    m_plugins->shutdown();
    m_networkManager->shutdown();

    DataPaths::clearTempData();

    qzSettings->saveSettings();
    QFile::remove(DataPaths::currentProfilePath() + QLatin1String("/WebpageIcons.db"));

#ifndef PORTABLE_BUILD
    sessionManager()->saveSettings();
#endif
}

void MainApplication::messageReceived(const QString &message)
{
    QWidget* actWin = getWindow();
    QUrl actUrl;

    if (message.startsWith(QLatin1String("URL:"))) {
        const QUrl url = QUrl::fromUserInput(message.mid(4));
        addNewTab(url);
        actWin = getWindow();
    }
    else if (message.startsWith(QLatin1String("ACTION:"))) {
        const QString text = message.mid(7);
        if (text == QLatin1String("NewTab")) {
            addNewTab();
        }
        else if (text == QLatin1String("NewWindow")) {
            actWin = createWindow(Qz::BW_NewWindow);
        }
        else if (text == QLatin1String("ShowDownloadManager")) {
            downloadManager()->show();
            actWin = downloadManager();
        }
        else if (text == QLatin1String("ToggleFullScreen") && actWin) {
            BrowserWindow* qz = static_cast<BrowserWindow*>(actWin);
            qz->toggleFullScreen();
        }
        else if (text.startsWith(QLatin1String("OpenUrlInCurrentTab"))) {
            actUrl = QUrl::fromUserInput(text.mid(19));
        }
        else if (text.startsWith(QLatin1String("OpenUrlInNewWindow"))) {
            createWindow(Qz::BW_NewWindow, QUrl::fromUserInput(text.mid(18)));
            return;
        }
    }
    else {
        // User attempted to start another instance, let's open a new window
        actWin = createWindow(Qz::BW_NewWindow);
    }

    if (!actWin) {
        if (!isClosing()) {
            // It can only occur if download manager window was still opened
            createWindow(Qz::BW_NewWindow, actUrl);
        }
        return;
    }

    actWin->setWindowState(actWin->windowState() & ~Qt::WindowMinimized);
    actWin->raise();
    actWin->activateWindow();
    actWin->setFocus();

    BrowserWindow* win = qobject_cast<BrowserWindow*>(actWin);

    if (win && !actUrl.isEmpty()) {
        win->loadAddress(actUrl);
    }
}

void MainApplication::windowDestroyed(QObject* window)
{
    // qobject_cast doesn't work because QObject::destroyed is emitted from destructor
    Q_ASSERT(static_cast<BrowserWindow*>(window));
    Q_ASSERT(m_windows.contains(static_cast<BrowserWindow*>(window)));

    m_windows.removeOne(static_cast<BrowserWindow*>(window));
}

void MainApplication::onFocusChanged()
{
    BrowserWindow* activeBrowserWindow = qobject_cast<BrowserWindow*>(activeWindow());

    if (activeBrowserWindow) {
        m_lastActiveWindow = activeBrowserWindow;

        emit activeWindowChanged(m_lastActiveWindow);
    }
}

void MainApplication::runDeferredPostLaunchActions()
{
    checkDefaultWebBrowser();
    checkOptimizeDatabase();
}

void MainApplication::downloadRequested(QWebEngineDownloadItem *download)
{
    downloadManager()->download(download);
}

void MainApplication::loadSettings()
{
    std::cerr << "[MAINAPPLICATION] loadSettings() started" << std::endl;
    Settings settings;
    settings.beginGroup("Themes");
    QString activeTheme = settings.value("activeTheme", DEFAULT_THEME_NAME).toString();
    settings.endGroup();

    std::cerr << "[MAINAPPLICATION] loadSettings() loading theme: " << activeTheme.toStdString() << std::endl;
    loadTheme(activeTheme);

    std::cerr << "[MAINAPPLICATION] loadSettings() getting web settings" << std::endl;
    QWebEngineSettings* webSettings = m_webProfile->settings();
    std::cerr << "[MAINAPPLICATION] loadSettings() web settings obtained" << std::endl;

    // Web browsing settings
    settings.beginGroup("Web-Browser-Settings");

    webSettings->setAttribute(QWebEngineSettings::LocalStorageEnabled, settings.value("HTML5StorageEnabled", true).toBool());
    webSettings->setAttribute(QWebEngineSettings::PluginsEnabled, settings.value("allowPlugins", true).toBool());
    webSettings->setAttribute(QWebEngineSettings::JavascriptEnabled, settings.value("allowJavaScript", true).toBool());
    webSettings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, settings.value("allowJavaScriptOpenWindow", false).toBool());
    webSettings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, settings.value("allowJavaScriptAccessClipboard", true).toBool());
    webSettings->setAttribute(QWebEngineSettings::LinksIncludedInFocusChain, settings.value("IncludeLinkInFocusChain", false).toBool());
    webSettings->setAttribute(QWebEngineSettings::XSSAuditingEnabled, settings.value("XSSAuditing", false).toBool());
    webSettings->setAttribute(QWebEngineSettings::PrintElementBackgrounds, settings.value("PrintElementBackground", true).toBool());
    webSettings->setAttribute(QWebEngineSettings::SpatialNavigationEnabled, settings.value("SpatialNavigation", false).toBool());
    webSettings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, settings.value("AnimateScrolling", true).toBool());
    webSettings->setAttribute(QWebEngineSettings::AutoLoadImages, settings.value("autoLoadImages", true).toBool());
    webSettings->setAttribute(QWebEngineSettings::HyperlinkAuditingEnabled, false);
    webSettings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    webSettings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    webSettings->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, false);

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    webSettings->setUnknownUrlSchemePolicy(QWebEngineSettings::AllowAllUnknownUrlSchemes);
#endif
    webSettings->setDefaultTextEncoding(settings.value("DefaultEncoding", webSettings->defaultTextEncoding()).toString());

    setWheelScrollLines(settings.value("wheelScrollLines", wheelScrollLines()).toInt());

    const QString userCss = settings.value("userStyleSheet", QString()).toString();
    settings.endGroup();

    setUserStyleSheet(userCss);

    settings.beginGroup("Browser-Fonts");
    webSettings->setFontFamily(QWebEngineSettings::StandardFont, settings.value("StandardFont", webSettings->fontFamily(QWebEngineSettings::StandardFont)).toString());
    webSettings->setFontFamily(QWebEngineSettings::CursiveFont, settings.value("CursiveFont", webSettings->fontFamily(QWebEngineSettings::CursiveFont)).toString());
    webSettings->setFontFamily(QWebEngineSettings::FantasyFont, settings.value("FantasyFont", webSettings->fontFamily(QWebEngineSettings::FantasyFont)).toString());
    webSettings->setFontFamily(QWebEngineSettings::FixedFont, settings.value("FixedFont", webSettings->fontFamily(QWebEngineSettings::FixedFont)).toString());
    webSettings->setFontFamily(QWebEngineSettings::SansSerifFont, settings.value("SansSerifFont", webSettings->fontFamily(QWebEngineSettings::SansSerifFont)).toString());
    webSettings->setFontFamily(QWebEngineSettings::SerifFont, settings.value("SerifFont", webSettings->fontFamily(QWebEngineSettings::SerifFont)).toString());
    webSettings->setFontSize(QWebEngineSettings::DefaultFontSize, settings.value("DefaultFontSize", 15).toInt());
    webSettings->setFontSize(QWebEngineSettings::DefaultFixedFontSize, settings.value("FixedFontSize", 14).toInt());
    webSettings->setFontSize(QWebEngineSettings::MinimumFontSize, settings.value("MinimumFontSize", 3).toInt());
    webSettings->setFontSize(QWebEngineSettings::MinimumLogicalFontSize, settings.value("MinimumLogicalFontSize", 5).toInt());
    settings.endGroup();

#ifdef PORTABLE_BUILD
    // For portable build (webOS), use memory-only HTTP cache and optimize for limited resources
    QWebEngineProfile* profile = m_webProfile;
    profile->setPersistentCookiesPolicy(QWebEngineProfile::AllowPersistentCookies);
    profile->setPersistentStoragePath(DataPaths::currentProfilePath());
    profile->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
    profile->setHttpCacheMaximumSize(20 * 1024 * 1024);  // 20MB memory cache limit for webOS

    // Disable animations to reduce CPU usage on webOS
    webSettings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, false);
    // Disable print element backgrounds (printing not supported on webOS)
    webSettings->setAttribute(QWebEngineSettings::PrintElementBackgrounds, false);
#else
    QWebEngineProfile* profile = QWebEngineProfile::defaultProfile();
    profile->setPersistentCookiesPolicy(QWebEngineProfile::AllowPersistentCookies);
    profile->setPersistentStoragePath(DataPaths::currentProfilePath());

    QString defaultPath = DataPaths::path(DataPaths::Cache);
    if (!defaultPath.startsWith(DataPaths::currentProfilePath()))
        defaultPath.append(QLatin1Char('/') + ProfileManager::currentProfile());
    const QString &cachePath = settings.value("Web-Browser-Settings/CachePath", defaultPath).toString();
    profile->setCachePath(cachePath);

    const bool allowCache = settings.value(QSL("Web-Browser-Settings/AllowLocalCache"), true).toBool();
    profile->setHttpCacheType(allowCache ? QWebEngineProfile::DiskHttpCache : QWebEngineProfile::MemoryHttpCache);

    const int cacheSize = settings.value(QSL("Web-Browser-Settings/LocalCacheSize"), 50).toInt() * 1000 * 1000;
    profile->setHttpCacheMaximumSize(cacheSize);

    settings.beginGroup(QSL("SpellCheck"));
    profile->setSpellCheckEnabled(settings.value(QSL("Enabled"), false).toBool());
    profile->setSpellCheckLanguages(settings.value(QSL("Languages")).toStringList());
    settings.endGroup();
#endif

    if (isPrivate()) {
        webSettings->setAttribute(QWebEngineSettings::LocalStorageEnabled, false);
        history()->setSaving(false);
    }

    if (m_downloadManager) {
        m_downloadManager->loadSettings();
    }

    std::cerr << "[MAINAPPLICATION] loadSettings() loading qzSettings" << std::endl;
    qzSettings->loadSettings();
    std::cerr << "[MAINAPPLICATION] loadSettings() loading userAgentManager" << std::endl;
    userAgentManager()->loadSettings();
    std::cerr << "[MAINAPPLICATION] loadSettings() loading networkManager" << std::endl;
    networkManager()->loadSettings();
    std::cerr << "[MAINAPPLICATION] loadSettings() completed" << std::endl;
}

void MainApplication::loadTheme(const QString &name)
{
    QString activeThemePath;
    const QStringList themePaths = DataPaths::allPaths(DataPaths::Themes);

    foreach (const QString &path, themePaths) {
        const QString theme = QString("%1/%2").arg(path, name);
        if (QFile::exists(theme + QLatin1String("/main.css"))) {
            activeThemePath = theme;
            break;
        }
    }

    if (activeThemePath.isEmpty()) {
        qWarning() << "Cannot load theme " << name;
        activeThemePath = QString("%1/%2").arg(DataPaths::path(DataPaths::Themes), DEFAULT_THEME_NAME);
    }

    QString qss = QzTools::readAllFileContents(activeThemePath + QLatin1String("/main.css"));

    // webOS is Linux-based
    qss.append(QzTools::readAllFileContents(activeThemePath + QLatin1String("/linux.css")));

    if (isRightToLeft()) {
        qss.append(QzTools::readAllFileContents(activeThemePath + QLatin1String("/rtl.css")));
    }

    if (isPrivate()) {
        qss.append(QzTools::readAllFileContents(activeThemePath + QLatin1String("/private.css")));
    }

    qss.append(QzTools::readAllFileContents(DataPaths::currentProfilePath() + QL1S("/userChrome.css")));

    QString relativePath = QDir::current().relativeFilePath(activeThemePath);
    qss.replace(QzRegExp(QSL("url\\s*\\(\\s*([^\\*:\\);]+)\\s*\\)"), Qt::CaseSensitive), QString("url(%1/\\1)").arg(relativePath));
    setStyleSheet(qss);
}

void MainApplication::translateApp()
{
    QString file = Settings().value(QSL("Language/language"), QLocale::system().name()).toString();

    // It can only be "C" locale, for which we will use default English language
    if (file.size() < 2)
        file.clear();

    if (!file.isEmpty() && !file.endsWith(QL1S(".qm")))
        file.append(QL1S(".qm"));

    // Either we load default language (with empty file), or we attempt to load xx.qm (xx_yy.qm)
    Q_ASSERT(file.isEmpty() || file.size() >= 5);

    QString translationPath = DataPaths::path(DataPaths::Translations);

    if (!file.isEmpty()) {
        const QStringList translationsPaths = DataPaths::allPaths(DataPaths::Translations);

        foreach (const QString &path, translationsPaths) {
            // If "xx_yy" translation doesn't exists, try to use "xx*" translation
            // It can only happen when language is chosen from system locale

            if (!QFile(QString("%1/%2").arg(path, file)).exists()) {
                QDir dir(path);
                QString lang = file.left(2) + QL1S("*.qm");

                const QStringList translations = dir.entryList(QStringList(lang));

                // If no translation can be found, default English will be used
                file = translations.isEmpty() ? QString() : translations.at(0);
            }

            if (!file.isEmpty() && QFile(QString("%1/%2").arg(path, file)).exists()) {
                translationPath = path;
                break;
            }
        }
    }

    // Load application translation
    QTranslator* app = new QTranslator(this);
    app->load(file, translationPath);

    // Load Qt translation (first try to load from Qt path)
    QTranslator* sys = new QTranslator(this);
    sys->load(QL1S("qt_") + file, QLibraryInfo::location(QLibraryInfo::TranslationsPath));

    // If there is no translation in Qt path for specified language, try to load it from our path
    if (sys->isEmpty()) {
        sys->load(QL1S("qt_") + file, translationPath);
    }

    m_languageFile = file;

    installTranslator(app);
    installTranslator(sys);
}

void MainApplication::checkDefaultWebBrowser()
{
    // Default browser check only applies to desktop platforms
    // webOS handles app launching through Luna Service Bus
}

void MainApplication::checkOptimizeDatabase()
{
    Settings settings;
    settings.beginGroup(QSL("Browser"));
    const int numberOfRuns = settings.value(QSL("RunsWithoutOptimizeDb"), 0).toInt();
    settings.setValue(QSL("RunsWithoutOptimizeDb"), numberOfRuns + 1);

    if (numberOfRuns > 20) {
        std::cout << "Optimizing database..." << std::endl;
        IconProvider::instance()->clearOldIconsInDatabase();
        settings.setValue(QSL("RunsWithoutOptimizeDb"), 0);
    }

    settings.endGroup();
}

void MainApplication::setupUserScripts()
{
    // WebChannel for SafeJsWorld
    QWebEngineScript script;
    script.setName(QSL("_qupzilla_webchannel"));
    script.setInjectionPoint(QWebEngineScript::DocumentCreation);
    script.setWorldId(WebPage::SafeJsWorld);
    script.setRunsOnSubFrames(true);
    script.setSourceCode(Scripts::setupWebChannel(script.worldId()));
    m_webProfile->scripts()->insert(script);

    // WebChannel for UnsafeJsWorld
    QWebEngineScript script2;
    script2.setName(QSL("_qupzilla_webchannel2"));
    script2.setInjectionPoint(QWebEngineScript::DocumentCreation);
    script2.setWorldId(WebPage::UnsafeJsWorld);
    script2.setRunsOnSubFrames(true);
    script2.setSourceCode(Scripts::setupWebChannel(script2.worldId()));
    m_webProfile->scripts()->insert(script2);

    // document.window object addons
    QWebEngineScript script3;
    script3.setName(QSL("_qupzilla_window_object"));
    script3.setInjectionPoint(QWebEngineScript::DocumentCreation);
    script3.setWorldId(WebPage::UnsafeJsWorld);
    script3.setRunsOnSubFrames(true);
    script3.setSourceCode(Scripts::setupWindowObject());
    m_webProfile->scripts()->insert(script3);
}

void MainApplication::setUserStyleSheet(const QString &filePath)
{
    QString userCss;
    userCss += QzTools::readAllFileContents(filePath).remove(QLatin1Char('\n'));

    const QString name = QStringLiteral("_qupzilla_userstylesheet");

    QWebEngineScript oldScript = m_webProfile->scripts()->findScript(name);
    if (!oldScript.isNull()) {
        m_webProfile->scripts()->remove(oldScript);
    }

    if (userCss.isEmpty())
        return;

    QWebEngineScript script;
    script.setName(name);
    script.setInjectionPoint(QWebEngineScript::DocumentReady);
    script.setWorldId(WebPage::SafeJsWorld);
    script.setRunsOnSubFrames(true);
    script.setSourceCode(Scripts::setCss(userCss));
    m_webProfile->scripts()->insert(script);
}

void MainApplication::createJumpList()
{
    // Jump lists are a Windows-only feature
}

void MainApplication::initPulseSupport()
{
    qputenv("PULSE_PROP_OVERRIDE_application.name", "QupZilla");
    qputenv("PULSE_PROP_OVERRIDE_application.icon_name", "qupzilla");
    qputenv("PULSE_PROP_OVERRIDE_media.icon_name", "qupzilla");
}
