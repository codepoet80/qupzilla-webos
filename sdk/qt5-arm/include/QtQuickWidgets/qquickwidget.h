// Qt Quick Widget stub for cross-compilation
#ifndef QQUICKWIDGET_H
#define QQUICKWIDGET_H

#include <QtWidgets/QWidget>
#include <QtQuick/QQuickWindow>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlError>
#include <QtGui/QSurfaceFormat>

QT_BEGIN_NAMESPACE

class QQmlComponent;
class QQuickItem;

class Q_DECL_EXPORT QQuickWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QQuickWidget(QWidget *parent = nullptr) : QWidget(parent), m_quickWindow(new QQuickWindow()) {}
    explicit QQuickWidget(QQmlEngine *engine, QWidget *parent) : QWidget(parent), m_quickWindow(new QQuickWindow()) { Q_UNUSED(engine); }
    explicit QQuickWidget(const QUrl &source, QWidget *parent = nullptr) : QWidget(parent), m_quickWindow(new QQuickWindow()) { Q_UNUSED(source); }
    virtual ~QQuickWidget() { delete m_quickWindow; }

    enum ResizeMode { SizeViewToRootObject, SizeRootObjectToView };
    enum Status { Null, Ready, Loading, Error };

    QUrl source() const { return QUrl(); }
    void setSource(const QUrl &) {}
    QQmlEngine *engine() const { return nullptr; }
    QQmlContext *rootContext() const { return nullptr; }
    QQuickItem *rootObject() const { return nullptr; }
    ResizeMode resizeMode() const { return SizeRootObjectToView; }
    void setResizeMode(ResizeMode) {}
    Status status() const { return Null; }
    QList<QQmlError> errors() const { return QList<QQmlError>(); }
    QSize sizeHint() const override { return QSize(); }
    QSize initialSize() const { return QSize(); }
    void setContent(const QUrl &, QQmlComponent *, QQuickItem *) {}
    void setClearColor(const QColor &) {}
    QQuickWindow::CreateTextureOption quickWindowCreateTextureOptions() const { return QQuickWindow::TextureHasAlphaChannel; }
    void setQuickWindowCreateTextureOptions(QQuickWindow::CreateTextureOptions) {}
    
    // Methods needed by QtWebEngine
    void setFormat(const QSurfaceFormat &) {}
    QSurfaceFormat format() const { return QSurfaceFormat(); }
    QQuickWindow *quickWindow() const { return m_quickWindow; }

    // grabFramebuffer - added in Qt 5.9.4
    QImage grabFramebuffer() const { return QImage(); }

public Q_SLOTS:
    void setResizeMode(int) {}

Q_SIGNALS:
    void statusChanged(QQuickWidget::Status);
    void sceneGraphError(QQuickWindow::SceneGraphError, const QString &);

private:
    QQuickWindow *m_quickWindow;
};

QT_END_NAMESPACE

#endif // QQUICKWIDGET_H
