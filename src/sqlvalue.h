#ifndef SQLVALUE_H
#define SQLVALUE_H

#include <QVariant>

#include <optional>

namespace lpd8 {

// Reads a value of an SQLite column as an integer from 0 to 'max', or
// returns nothing.
//
// Tables created before 2018 have no CHECK constraints, so a column can
// hold anything. Only a value stored as an INTEGER is taken, range-checked
// as a 64-bit number:
// - QVariant::toInt() can't be used: it wraps (4294967336 becomes 40) and
//   rounds (36.5 becomes 37), and still reports success.
// - A REAL is refused. INTEGER column affinity already stores an integral
//   value such as 36.0 as the INTEGER 36, so a REAL that is left is not an
//   integer (36.5).
// - TEXT is refused for the same reason: affinity turns '36' into 36, so a
//   TEXT that is left is not a number.
// - NULL is refused.
std::optional<int> sqlInteger(const QVariant &value, int max);

} // namespace lpd8

#endif // SQLVALUE_H
