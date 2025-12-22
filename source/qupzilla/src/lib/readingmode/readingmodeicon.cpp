/* ============================================================
* QupZilla - Qt web browser
* Copyright (C) 2024
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
#include "readingmodeicon.h"
#include "readingmodemanager.h"
#include "mainapplication.h"
#include "browserwindow.h"
#include "webview.h"
#include "webpage.h"

ReadingModeIcon::ReadingModeIcon(QObject *parent)
    : AbstractButtonInterface(parent)
{
    setTitle(tr("Reading Mode"));
    setIcon(QIcon(QSL(":readingmode/reader-icon.svg")));
    setActive(false);
    setToolTip(tr("Reading Mode unavailable"));

    connect(this, &AbstractButtonInterface::clicked, this, &ReadingModeIcon::clicked);
    connect(this, &AbstractButtonInterface::webViewChanged, this, &ReadingModeIcon::webViewChanged);
    connect(this, &AbstractButtonInterface::activeChanged, this, &ReadingModeIcon::updateActiveIcon);
    connect(ReadingModeManager::instance(), &ReadingModeManager::pageReadabilityChanged,
            this, [this](const QUrl &url, bool readable) {
                if (m_view && m_view->url() == url) {
                    setActive(readable);
                    setToolTip(readable ? tr("Enter Reading Mode") : tr("Reading Mode unavailable"));
                }
            });
}

QString ReadingModeIcon::id() const
{
    return QSL("readingmode-icon");
}

QString ReadingModeIcon::name() const
{
    return tr("Reading Mode");
}

void ReadingModeIcon::updateState()
{
    WebView *view = webView();
    if (!view) {
        setActive(false);
        setToolTip(name());
        return;
    }

    // Don't show for special schemes
    const QString scheme = view->url().scheme();
    if (scheme == QL1S("qupzilla") || scheme == QL1S("about") ||
        scheme == QL1S("data") || scheme == QL1S("file") ||
        scheme == QL1S("view-source")) {
        setActive(false);
        setToolTip(tr("Reading Mode unavailable"));
        return;
    }

    bool readable = ReadingModeManager::instance()->isPageReadable(view->url());
    setActive(readable);
    setToolTip(readable ? tr("Enter Reading Mode") : tr("Reading Mode unavailable"));
}

void ReadingModeIcon::webViewChanged(WebView *view)
{
    if (m_view) {
        disconnect(m_view.data(), &WebView::urlChanged, this, &ReadingModeIcon::updateState);
        disconnect(m_view.data(), &WebView::loadFinished, this, &ReadingModeIcon::pageLoadFinished);
    }

    m_view = view;

    if (m_view) {
        connect(m_view.data(), &WebView::urlChanged, this, &ReadingModeIcon::updateState);
        connect(m_view.data(), &WebView::loadFinished, this, &ReadingModeIcon::pageLoadFinished);
    }

    updateState();
}

void ReadingModeIcon::pageLoadFinished(bool ok)
{
    if (!ok || !m_view) {
        return;
    }

    // Don't run detection on special pages
    const QString scheme = m_view->url().scheme();
    if (scheme == QL1S("qupzilla") || scheme == QL1S("about") ||
        scheme == QL1S("data") || scheme == QL1S("file")) {
        return;
    }

    // Run detection script
    WebPage *page = m_view->page();
    if (page) {
        page->runJavaScript(ReadingModeManager::getDetectionScript(),
                           WebPage::SafeJsWorld,
                           [this](const QVariant &result) {
                               readabilityDetected(result);
                           });
    }
}

void ReadingModeIcon::readabilityDetected(const QVariant &result)
{
    if (!m_view) {
        return;
    }

    bool readable = result.toBool();
    ReadingModeManager::instance()->setPageReadable(m_view->url(), readable);
}

void ReadingModeIcon::clicked(ClickController *controller)
{
    Q_UNUSED(controller)

    WebView *view = webView();
    if (!view || !isActive()) {
        return;
    }

    // Extract content first, then navigate
    WebPage *page = view->page();
    if (!page) {
        return;
    }

    const QUrl originalUrl = view->url();

    page->runJavaScript(ReadingModeManager::getExtractionScript(),
                       WebPage::SafeJsWorld,
                       [this, view, originalUrl](const QVariant &result) {
                           if (!view) {
                               return;
                           }
                           bool success = contentExtracted(result, originalUrl);
                           if (success) {
                               // Navigate to reader view
                               QUrl readerUrl = ReadingModeManager::readerUrl(originalUrl);
                               view->load(readerUrl);
                           }
                       });
}

bool ReadingModeIcon::contentExtracted(const QVariant &result, const QUrl &originalUrl)
{
    if (!result.isValid() || result.type() != QVariant::Map) {
        qWarning() << "ReadingMode: Extraction returned invalid result";
        return false;
    }

    QVariantMap map = result.toMap();

    ReadingModeArticle article;
    article.title = map.value(QSL("title")).toString();
    article.content = map.value(QSL("content")).toString();
    article.byline = map.value(QSL("byline")).toString();
    article.originalUrl = originalUrl;

    if (article.isValid()) {
        ReadingModeManager::instance()->cacheArticle(article);
        return true;
    }

    qWarning() << "ReadingMode: Extracted article is not valid - title:" << article.title.left(50) << "content length:" << article.content.length();
    return false;
}

void ReadingModeIcon::updateActiveIcon(bool active)
{
    if (active) {
        setIcon(QIcon(QSL(":readingmode/reader-icon-active.svg")));
    } else {
        setIcon(QIcon(QSL(":readingmode/reader-icon.svg")));
    }
}
