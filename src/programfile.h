#ifndef PROGRAMFILE_H
#define PROGRAMFILE_H

#include "program.h"

#include <QString>

#include <expected>

namespace lpd8 {

// A program read from a file, and the name the file gives it.
struct ImportedProgram
{
    QString name;
    Program program;
};

// Program files (.lpd8) hold one program message in the storage format
// (doc/SYSEX.md, "Storage format"), as decimal numbers separated by spaces.
// These functions read and write them for ProgramStore, in the same format
// byte for byte as the SQLite version did (tests/fixtures/README.md).

// Reads the program in 'path'. Its name is the file's name without the
// extension: 'Drums.lpd8' becomes 'Drums'. The error is a message for the
// user, naming the file.
std::expected<ImportedProgram, QString> importProgramFile(const QString &path);

// Writes 'program' to 'path', atomically: a failed export never leaves a
// truncated file, nor destroys the file it was replacing. The error is a
// message for the user.
std::expected<void, QString> exportProgramFile(const Program &program, const QString &path);

} // namespace lpd8

#endif // PROGRAMFILE_H
