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
#ifndef READINGMODEMANAGER_H
#define READINGMODEMANAGER_H

#include <QObject>
#include <QUrl>
#include <QHash>
#include <functional>

#include "qzcommon.h"

class WebPage;

struct ReadingModeArticle {
    QString title;
    QString content;
    QString byline;
    QUrl originalUrl;
    bool isValid() const { return !content.isEmpty(); }
};

class QUPZILLA_EXPORT ReadingModeManager : public QObject
{
    Q_OBJECT

public:
    static ReadingModeManager* instance();

    // Check if page appears to have readable content
    bool isPageReadable(const QUrl &url) const;
    void setPageReadable(const QUrl &url, bool readable);

    // Cache extracted article content
    void cacheArticle(const ReadingModeArticle &article);
    ReadingModeArticle getCachedArticle(const QUrl &url) const;
    void clearCache();

    // Generate reader URL for a page
    static QUrl readerUrl(const QUrl &originalUrl);
    static QUrl originalUrlFromReader(const QUrl &readerUrl);

    // JavaScript for readability detection
    static QString getDetectionScript();

    // JavaScript for content extraction (includes Readability.js)
    static QString getExtractionScript();

signals:
    void pageReadabilityChanged(const QUrl &url, bool readable);

private:
    explicit ReadingModeManager(QObject *parent = nullptr);

    QHash<QUrl, bool> m_readablePages;
    QHash<QUrl, ReadingModeArticle> m_cachedArticles;

    static ReadingModeManager *s_instance;
};

#endif // READINGMODEMANAGER_H
