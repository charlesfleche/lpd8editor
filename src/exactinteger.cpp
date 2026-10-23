#include "exactinteger.h"

#include <QMetaType>

namespace lpd8 {

std::optional<int> exactInteger(const QVariant &value, int max)
{
    if (value.isNull()) {
        return std::nullopt;
    }
    bool isInteger = false;
    qlonglong v = 0;
    switch (value.typeId()) {
    case QMetaType::Int:
    case QMetaType::LongLong: // what QSQLITE returns for INTEGER
        v = value.toLongLong(&isInteger);
        break;
    case QMetaType::UInt:
    case QMetaType::ULongLong: {
        const qulonglong u = value.toULongLong(&isInteger);
        if (u > static_cast<qulonglong>(max)) {
            return std::nullopt;
        }
        v = static_cast<qlonglong>(u);
        break;
    }
    default: // REAL, TEXT, BLOB: see the header
        return std::nullopt;
    }
    if (!isInteger || v < 0 || v > max) {
        return std::nullopt;
    }
    return static_cast<int>(v);
}

} // namespace lpd8
