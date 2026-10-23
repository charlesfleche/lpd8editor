#include "programfile.h"

#include "lpd8codec.h"
#include "utils.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace lpd8 {

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("ProgramFile", text);
}

} // namespace

std::expected<ImportedProgram, QString> importProgramFile(const QString &path)
{
    const QString file = QDir::toNativeSeparators(path);
    const auto sysex = fromSysexTextFile(path);
    if (!sysex) {
        return std::unexpected(sysex.error());
    }

    const auto decoded = decodeProgram(*sysex);
    if (!decoded) {
        if (decoded.error().kind == DecodeError::Kind::NotAProgram) {
            // The same message as before the switch to ProgramStore.
            return std::unexpected(tr("%1 is not an LPD8 program.").arg(file));
        }
        // Positions count from 1 here and in fromSysexTextFile()'s messages:
        // the same value of the file has the same position in both.
        return std::unexpected(tr("%1: %2").arg(file, describe(decoded.error())));
    }

    // The slot named in the message is not part of the program: a program
    // file can be sent to any slot.
    return ImportedProgram{.name = QFileInfo(path).completeBaseName(), .program = decoded->program};
}

std::expected<void, QString> exportProgramFile(const Program &program, const QString &path)
{
    return writeProgramFile(encodeStoredProgram(program), path);
}

} // namespace lpd8
