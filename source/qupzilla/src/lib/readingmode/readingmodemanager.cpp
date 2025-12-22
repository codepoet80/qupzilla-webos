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
#include "readingmodemanager.h"
#include "qztools.h"
#include "webpage.h"

#include <QUrlQuery>

ReadingModeManager* ReadingModeManager::s_instance = nullptr;

ReadingModeManager* ReadingModeManager::instance()
{
    if (!s_instance) {
        s_instance = new ReadingModeManager();
    }
    return s_instance;
}

ReadingModeManager::ReadingModeManager(QObject *parent)
    : QObject(parent)
{
}

bool ReadingModeManager::isPageReadable(const QUrl &url) const
{
    return m_readablePages.value(url, false);
}

void ReadingModeManager::setPageReadable(const QUrl &url, bool readable)
{
    if (m_readablePages.value(url) != readable) {
        m_readablePages[url] = readable;
        emit pageReadabilityChanged(url, readable);
    }
}

void ReadingModeManager::cacheArticle(const ReadingModeArticle &article)
{
    if (article.isValid()) {
        m_cachedArticles[article.originalUrl] = article;
    }
}

ReadingModeArticle ReadingModeManager::getCachedArticle(const QUrl &url) const
{
    return m_cachedArticles.value(url);
}

void ReadingModeManager::clearCache()
{
    m_cachedArticles.clear();
    m_readablePages.clear();
}

QUrl ReadingModeManager::readerUrl(const QUrl &originalUrl)
{
    QUrl url(QSL("qupzilla:reader"));
    QUrlQuery query;
    query.addQueryItem(QSL("url"), QString::fromUtf8(originalUrl.toEncoded().toBase64()));
    url.setQuery(query);
    return url;
}

QUrl ReadingModeManager::originalUrlFromReader(const QUrl &readerUrl)
{
    QUrlQuery query(readerUrl);
    QString encodedUrl = query.queryItemValue(QSL("url"));
    if (!encodedUrl.isEmpty()) {
        return QUrl::fromEncoded(QByteArray::fromBase64(encodedUrl.toUtf8()));
    }
    return QUrl();
}

QString ReadingModeManager::getDetectionScript()
{
    // Heuristic detection for article-like content
    // Checks for: article tags, main content areas, sufficient paragraph text
    return QSL(
        "(function() {"
        "  try {"
        "    var dominated = false;"
        "    var article = document.querySelector('article, [itemtype*=\"Article\"], [itemtype*=\"NewsArticle\"], [itemtype*=\"BlogPosting\"]');"
        "    if (article) dominated = true;"
        "    var main = document.querySelector('main, [role=\"main\"], .post, .article, .entry-content, .post-content');"
        "    if (main) dominated = true;"
        "    var paragraphs = document.querySelectorAll('p');"
        "    var totalTextLength = 0;"
        "    var longParagraphs = 0;"
        "    for (var i = 0; i < paragraphs.length && i < 30; i++) {"
        "      var text = paragraphs[i].textContent.trim();"
        "      totalTextLength += text.length;"
        "      if (text.length > 100) longParagraphs++;"
        "    }"
        "    var hasEnoughText = totalTextLength > 500 && longParagraphs >= 2;"
        "    return dominated || hasEnoughText;"
        "  } catch(e) {"
        "    return false;"
        "  }"
        "})()");
}

QString ReadingModeManager::getExtractionScript()
{
    // Simple ES5-compatible article extraction
    // TODO: Add ES5-transpiled Readability.js for better extraction
    return QSL(
        "(function() {"
        "  var title = document.title || '';"
        "  var content = '';"
        "  var byline = '';"
        "  try {"
        // Try to find article container
        "    var article = document.querySelector('article');"
        "    if (!article) article = document.querySelector('[role=\"article\"]');"
        "    if (!article) article = document.querySelector('.post-content');"
        "    if (!article) article = document.querySelector('.entry-content');"
        "    if (!article) article = document.querySelector('.article-content');"
        "    if (!article) article = document.querySelector('.article-body');"
        "    if (!article) article = document.querySelector('.post');"
        "    if (!article) article = document.querySelector('.article');"
        "    if (!article) article = document.querySelector('main');"
        "    if (!article) article = document.querySelector('[role=\"main\"]');"
        "    if (article) {"
        // Clone and clean up the article
        "      var clone = article.cloneNode(true);"
        // Remove scripts, styles, nav, ads, etc.
        "      var removes = clone.querySelectorAll('script, style, nav, aside, .ad, .ads, .advertisement, .social, .share, .comments, .sidebar, footer, header, .nav, .menu, .related, .recommended');"
        "      for (var i = 0; i < removes.length; i++) {"
        "        removes[i].parentNode.removeChild(removes[i]);"
        "      }"
        "      content = clone.innerHTML;"
        "    } else {"
        // Fallback: collect paragraphs
        "      var paras = document.querySelectorAll('p');"
        "      var htmlParts = [];"
        "      for (var i = 0; i < paras.length; i++) {"
        "        var text = paras[i].textContent || '';"
        "        if (text.replace(/\\s+/g, ' ').trim().length > 80) {"
        "          htmlParts.push('<p>' + paras[i].innerHTML + '</p>');"
        "        }"
        "      }"
        "      content = htmlParts.join('');"
        "    }"
        // Try to get better title from h1 or og:title
        "    var h1 = document.querySelector('article h1, main h1, .post h1, h1');"
        "    if (h1) title = h1.textContent || title;"
        "    var ogTitle = document.querySelector('meta[property=\"og:title\"]');"
        "    if (ogTitle && ogTitle.getAttribute('content')) title = ogTitle.getAttribute('content');"
        // Get author
        "    var metaAuthor = document.querySelector('meta[name=\"author\"]');"
        "    if (metaAuthor) byline = metaAuthor.getAttribute('content') || '';"
        "    if (!byline) {"
        "      var authorEl = document.querySelector('.author, .byline, [rel=\"author\"]');"
        "      if (authorEl) byline = authorEl.textContent || '';"
        "    }"
        "  } catch(e) {"
        "    console.error('Reading mode extraction error:', e);"
        "  }"
        "  return { title: title, content: content, byline: byline };"
        "})()");
}
