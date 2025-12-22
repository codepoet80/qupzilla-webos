// Qt 5.9.7 compatibility for Qt 5.15 moc output
// This header provides compatibility types for newer moc output to compile
// against Qt 5.9.7 headers.

#ifndef QT59_MOC_COMPAT_H
#define QT59_MOC_COMPAT_H

#include <QtCore/qobjectdefs.h>

// In Qt 5.12+, QMetaObject has a nested SuperData struct.
// In Qt 5.9.7, the superdata member is just a const QMetaObject*.
// We add a compatibility struct inside QMetaObject's namespace.

namespace {
    // SuperData compatibility - converts newer moc output to Qt 5.9.7 format
    struct Qt5MetaObjectSuperData {
        const QMetaObject *direct;

        constexpr Qt5MetaObjectSuperData(std::nullptr_t) : direct(nullptr) {}
        constexpr Qt5MetaObjectSuperData(const QMetaObject *mo) : direct(mo) {}
        constexpr operator const QMetaObject *() const { return direct; }

        template <const QMetaObject &MO>
        static constexpr const QMetaObject* link() { return &MO; }
    };
}

// Inject SuperData into QMetaObject namespace if not present
// This uses a macro trick since Qt 5.9.7 doesn't have SuperData
#ifndef QT_FEATURE_cxx17_overload_resolution_fixes
namespace QMetaObject_compat {
    using SuperData = Qt5MetaObjectSuperData;
}
// Use preprocessor to redirect QMetaObject::SuperData to our compat version
#define QMetaObject_SuperData_link QMetaObject_compat::SuperData::link
#endif

#endif // QT59_MOC_COMPAT_H
