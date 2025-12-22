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
#ifndef READINGMODEICON_H
#define READINGMODEICON_H

#include <QPointer>
#include <QUrl>

#include "qzcommon.h"
#include "abstractbuttoninterface.h"

class QUPZILLA_EXPORT ReadingModeIcon : public AbstractButtonInterface
{
    Q_OBJECT

public:
    explicit ReadingModeIcon(QObject *parent = nullptr);

    QString id() const override;
    QString name() const override;

private:
    void updateState();
    void webViewChanged(WebView *view);
    void clicked(ClickController *controller);
    void pageLoadFinished(bool ok);
    void readabilityDetected(const QVariant &result);
    bool contentExtracted(const QVariant &result, const QUrl &originalUrl);
    void updateActiveIcon(bool active);

    QPointer<WebView> m_view;
};

#endif // READINGMODEICON_H
