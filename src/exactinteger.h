#ifndef EXACTINTEGER_H
#define EXACTINTEGER_H

#include <QVariant>

#include <optional>

namespace lpd8 {

// A QVariant as an integer from 0 to 'max', or nothing. Only a value that is
// stored as an integer type is taken, range-checked as a 64-bit number:
// QVariant::toInt() can't be used, because it wraps (4294967336 becomes 40)
// and rounds (36.5 becomes 37), and still reports success. REAL, text, BLOB
// and NULL values are refused.
//
// It has two uses, and both need this exactness:
// - Values read from SQLite (db.cpp, migration.cpp). Tables created before
//   2018 have no CHECK constraints, so a column can hold anything. INTEGER
//   column affinity stores 36.0 and '36' as the INTEGER 36, so a REAL or
//   TEXT that is left is not an integer.
// - Values from editors (programmodels.cpp). A QVariant can carry a 64-bit
//   value, and casting it would store a different, valid-looking value.
std::optional<int> exactInteger(const QVariant &value, int max);

} // namespace lpd8

#endif // EXACTINTEGER_H
