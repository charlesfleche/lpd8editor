#include "programstore.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSaveFile>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <initializer_list>
#include <system_error>
#include <utility>

using namespace Qt::StringLiterals; // "..."_L1

namespace lpd8 {

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("ProgramStore", text);
}

bool isAcceptableName(const QString &name)
{
    return name.isValidUtf16();
}

// --- Writing

QJsonObject padToJson(const Pad &pad)
{
    return {{"note", pad.note},
            {"programChange", pad.programChange},
            {"controlChange", pad.controlChange},
            {"toggle", pad.toggle}};
}

QJsonObject knobToJson(const Knob &knob)
{
    return {{"controlChange", knob.controlChange}, {"low", knob.low}, {"high", knob.high}};
}

QJsonObject programToJson(const NamedProgram &stored)
{
    QJsonArray pads;
    for (const Pad &pad : stored.program.pads) {
        pads.append(padToJson(pad));
    }
    QJsonArray knobs;
    for (const Knob &knob : stored.program.knobs) {
        knobs.append(knobToJson(knob));
    }
    return {{"name", stored.name},
            {"channel", stored.program.channel},
            {"pads", pads},
            {"knobs", knobs}};
}

// --- Reading. Each function reports what is wrong, naming where, so the
// user (or whoever edited the file) can find it.

using Error = std::unexpected<QString>;

// JSON has only one number type: an integer is a number without a
// fractional part. Returns the number if it is one.
std::optional<double> wholeNumber(const QJsonValue &value)
{
    if (!value.isDouble()) {
        return std::nullopt;
    }
    const double d = value.toDouble();
    if (!std::isfinite(d) || d != std::trunc(d)) {
        return std::nullopt;
    }
    return d;
}

// An error naming the first key of 'object' that isn't in 'known'.
std::optional<QString> unknownKey(const QJsonObject &object,
                                  std::initializer_list<QLatin1StringView> known)
{
    const QStringList keys = object.keys();
    const auto unknown = std::ranges::find_if(keys, [&known](const QString &key) {
        return std::ranges::find(known, key) == known.end();
    });
    if (unknown != keys.end()) {
        return tr("unknown key '%1'").arg(*unknown);
    }
    return std::nullopt;
}

std::expected<int, QString> integer(const QJsonObject &object, QLatin1StringView key, int max)
{
    const auto d = wholeNumber(object.value(key));
    if (!d || *d < 0 || *d > max) {
        return Error(tr("'%1' must be an integer from 0 to %2").arg(key, QString::number(max)));
    }
    return static_cast<int>(*d);
}

std::expected<quint8, QString> midiValue(const QJsonObject &object, QLatin1StringView key)
{
    return integer(object, key, midi_value_max).transform([](int v) {
        return static_cast<quint8>(v);
    });
}

std::expected<Pad, QString> padFromJson(const QJsonValue &value)
{
    if (!value.isObject()) {
        return Error(tr("a pad must be an object"));
    }
    const QJsonObject object = value.toObject();
    if (const auto unknown = unknownKey(
                object, {"note"_L1, "programChange"_L1, "controlChange"_L1, "toggle"_L1})) {
        return Error(*unknown);
    }
    const auto note = midiValue(object, "note"_L1);
    const auto programChange = midiValue(object, "programChange"_L1);
    const auto controlChange = midiValue(object, "controlChange"_L1);
    for (const auto *field : {&note, &programChange, &controlChange}) {
        if (!*field) {
            return Error(field->error());
        }
    }
    const QJsonValue toggle = object.value("toggle"_L1);
    if (!toggle.isBool()) {
        return Error(tr("'toggle' must be true or false"));
    }
    return Pad{.note = *note,
               .programChange = *programChange,
               .controlChange = *controlChange,
               .toggle = toggle.toBool()};
}

std::expected<Knob, QString> knobFromJson(const QJsonValue &value)
{
    if (!value.isObject()) {
        return Error(tr("a knob must be an object"));
    }
    const QJsonObject object = value.toObject();
    if (const auto unknown = unknownKey(object, {"controlChange"_L1, "low"_L1, "high"_L1})) {
        return Error(*unknown);
    }
    const auto controlChange = midiValue(object, "controlChange"_L1);
    const auto low = midiValue(object, "low"_L1);
    const auto high = midiValue(object, "high"_L1);
    for (const auto *field : {&controlChange, &low, &high}) {
        if (!*field) {
            return Error(field->error());
        }
    }
    return Knob{.controlChange = *controlChange, .low = *low, .high = *high};
}

// Reads the array 'key' of exactly N controls into 'controls'. 'key' is the
// JSON key, 'label' the translated name of one control for the messages
// (different types, so they can't be swapped by mistake).
template<typename Control, std::size_t N, typename FromJson>
std::expected<void, QString> controlsFromJson(const QJsonObject &object,
                                              QLatin1StringView key,
                                              const QString &label,
                                              std::array<Control, N> &controls,
                                              FromJson fromJson)
{
    const QJsonValue value = object.value(key);
    if (!value.isArray() || value.toArray().size() != static_cast<qsizetype>(N)) {
        return Error(tr("'%1' must be a list of %2").arg(key, QString::number(N)));
    }
    const QJsonArray array = value.toArray();
    qsizetype i = 0; // same size as 'controls', checked above
    for (Control &control : controls) {
        const auto read = fromJson(array.at(i));
        if (!read) {
            // Pads and knobs are numbered from 1, as on the device.
            return Error(tr("%1 %2: %3").arg(label, QString::number(i + 1), read.error()));
        }
        control = *read;
        ++i;
    }
    return {};
}

// 'number' is the program's position in the file, from 1: the file has no
// other way to tell two programs of the same name apart.
std::expected<NamedProgram, QString> programFromJson(const QJsonObject &object, qsizetype number)
{
    NamedProgram stored;

    const QJsonValue name = object.value("name"_L1);
    if (!name.isString()) {
        return Error(tr("program %1: 'name' must be a string").arg(number));
    }
    stored.name = name.toString();

    // From here, errors name the program.
    const auto inProgram = [&stored, number](const QString &error) {
        return Error(tr("program %1 (%2): %3").arg(QString::number(number), stored.name, error));
    };

    if (const auto unknown = unknownKey(object, {"name"_L1, "channel"_L1, "pads"_L1, "knobs"_L1})) {
        return inProgram(*unknown);
    }
    if (!isAcceptableName(stored.name)) {
        return inProgram(tr("invalid characters in the name"));
    }

    const auto channel = integer(object, "channel"_L1, midi_channel_max);
    if (!channel) {
        return inProgram(channel.error());
    }
    stored.program.channel = static_cast<quint8>(*channel);

    if (const auto pads =
                controlsFromJson(object, "pads"_L1, tr("pad"), stored.program.pads, padFromJson);
        !pads) {
        return inProgram(pads.error());
    }
    if (const auto knobs = controlsFromJson(
                object, "knobs"_L1, tr("knob"), stored.program.knobs, knobFromJson);
        !knobs) {
        return inProgram(knobs.error());
    }
    // Every field was checked above; this is the same rule as everywhere
    // else, so a rule added to isValid() applies to files too.
    if (!stored.program.isValid()) {
        return inProgram(tr("a value is out of range"));
    }
    return stored;
}

} // namespace

QByteArray toJson(const QList<NamedProgram> &programs)
{
    QJsonArray array;
    for (const NamedProgram &stored : programs) {
        array.append(programToJson(stored));
    }
    const QJsonObject root{{"format", store_format_version}, {"programs", array}};
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

std::expected<QList<NamedProgram>, LoadError> fromJson(const QByteArray &json)
{
    const auto damaged = [](const QString &why) {
        return std::unexpected(LoadError{.kind = LoadError::Kind::Damaged, .message = why});
    };
    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return damaged(tr("not valid JSON (%1 at offset %2)")
                               .arg(parseError.errorString(), QString::number(parseError.offset)));
    }
    if (!document.isObject()) {
        return damaged(tr("not an lpd8editor programs file"));
    }
    const QJsonObject root = document.object();

    const QJsonValue formatValue = root.value("format"_L1);
    if (!formatValue.isDouble()) {
        return damaged(tr("not an lpd8editor programs file (no format version)"));
    }
    // Compared as a number: a version that is not an integer, or doesn't fit
    // in an int, must not be read as 0.
    const double format = formatValue.toDouble();
    if (format > store_format_version) {
        // Never half-read a newer format: saving it back would lose what this
        // version doesn't know about.
        return std::unexpected(LoadError{
                .kind = LoadError::Kind::NewerFormat,
                .message = tr("written by a newer version of lpd8editor (format %1, this version "
                              "reads up to %2)")
                                   .arg(QString::number(format),
                                        QString::number(store_format_version))});
    }
    if (format != store_format_version) {
        return damaged(tr("unknown format version %1").arg(QString::number(format)));
    }
    if (const auto unknown = unknownKey(root, {"format"_L1, "programs"_L1})) {
        return damaged(*unknown);
    }

    const QJsonValue programsValue = root.value("programs"_L1);
    if (!programsValue.isArray()) {
        return damaged(tr("'programs' must be a list"));
    }

    QList<NamedProgram> programs;
    for (const QJsonValue value : programsValue.toArray()) {
        if (!value.isObject()) {
            return damaged(tr("each program must be an object"));
        }
        auto stored = programFromJson(value.toObject(), programs.size() + 1);
        if (!stored) {
            return damaged(stored.error());
        }
        programs.append(std::move(*stored));
    }
    return programs;
}

ProgramStore::ProgramStore(QString path, QObject *parent)
    : QObject(parent)
    , m_path(std::move(path))
{ }

ProgramStore::~ProgramStore() = default;

qsizetype ProgramStore::indexOf(int id) const
{
    const auto it = std::ranges::find(m_programs, id, &StoredProgram::id);
    return it == m_programs.cend() ? -1 : std::distance(m_programs.cbegin(), it);
}

std::optional<StoredProgram> ProgramStore::find(int id) const
{
    const qsizetype i = indexOf(id);
    if (i < 0) {
        return std::nullopt;
    }
    return m_programs.at(i);
}

const StoredProgram *ProgramStore::get(int id) const
{
    const qsizetype i = indexOf(id);
    return i < 0 ? nullptr : &m_programs.at(i);
}

std::optional<int> ProgramStore::rowOf(int id) const
{
    const qsizetype i = indexOf(id);
    if (i < 0) {
        return std::nullopt;
    }
    return static_cast<int>(i);
}

std::optional<int> ProgramStore::add(const QString &name, const Program &program, int id)
{
    return insert(static_cast<int>(m_programs.size()), name, program, id);
}

std::optional<int>
ProgramStore::insert(int row, const QString &name, const Program &program, int id)
{
    if (row < 0 || row > m_programs.size() || !program.isValid() || !isAcceptableName(name)) {
        return std::nullopt;
    }
    if (id == -1) {
        if (m_nextId > max_program_id) {
            return std::nullopt; // every id was handed out
        }
        id = m_nextId;
    } else if (id < 1 || id > max_program_id || indexOf(id) >= 0) {
        return std::nullopt;
    }
    // id <= max_program_id, so id + 1 can't overflow.
    m_nextId = std::max(m_nextId, id + 1);

    emit programAboutToBeAdded(id, row);
    m_programs.insert(row, StoredProgram{.id = id, .name = name, .program = program});
    emit programAdded(id, row);
    return id;
}

bool ProgramStore::remove(int id)
{
    const qsizetype i = indexOf(id);
    if (i < 0) {
        return false;
    }
    const auto row = static_cast<int>(i);
    emit programAboutToBeRemoved(id, row);
    m_programs.removeAt(i);
    emit programRemoved(id, row);
    return true;
}

bool ProgramStore::setProgram(int id, const Program &program)
{
    const qsizetype i = indexOf(id);
    if (i < 0 || !program.isValid()) {
        return false;
    }
    if (m_programs[i].program == program) {
        return true;
    }
    m_programs[i].program = program;
    emit programChanged(id, static_cast<int>(i));
    return true;
}

bool ProgramStore::setName(int id, const QString &name)
{
    const qsizetype i = indexOf(id);
    if (i < 0 || !isAcceptableName(name)) {
        return false;
    }
    if (m_programs[i].name == name) {
        return true;
    }
    m_programs[i].name = name;
    emit nameChanged(id, static_cast<int>(i));
    return true;
}

std::expected<void, LoadError> ProgramStore::load()
{
    const QString file = QDir::toNativeSeparators(m_path);
    // The caller checked that m_nextId leaves room for an id each.
    const auto replaceWith = [this](const QList<NamedProgram> &content) {
        emit programsAboutToBeReset();
        m_programs.clear();
        m_programs.reserve(content.size());
        for (const NamedProgram &named : content) {
            // New ids, above any handed out before: an id from before the
            // load can't come to name one of these programs.
            m_programs.append(
                    StoredProgram{.id = m_nextId++, .name = named.name, .program = named.program});
        }
        m_loadError.reset();
        emit programsReset();
    };

    // std::filesystem tells "not there" (ENOENT, ENOTDIR) apart from "can't
    // look" (EACCES on a folder above it, an I/O error): QFile::exists()
    // returns false for both, and a programs file behind a folder the user
    // can't enter would be taken for a first start.
    std::error_code statError;
    const auto status = std::filesystem::status(QFile::encodeName(m_path).toStdString(), statError);
    if (status.type() == std::filesystem::file_type::not_found) {
        // The first start: no programs yet. An empty store stays as it is,
        // without signals: a change that changes nothing emits nothing.
        m_loadError.reset();
        if (!m_programs.isEmpty()) {
            replaceWith({});
        }
        return {};
    }
    const auto unreadable = [this, &file](LoadError::Kind kind, const QString &why) {
        m_loadError = LoadError{
                .kind = kind, .message = tr("Cannot read the programs file %1: %2").arg(file, why)};
        return std::unexpected(*m_loadError);
    };
    if (statError) {
        return unreadable(LoadError::Kind::CannotRead, QString::fromLocal8Bit(statError.message()));
    }
    // Anything but a regular file can't be read as one. QFile::open()
    // already refuses a directory, but opening a FIFO would block until
    // something writes to it: the application would hang at startup.
    if (status.type() != std::filesystem::file_type::regular) {
        return unreadable(LoadError::Kind::CannotRead, tr("not a regular file"));
    }
    QFile f(m_path);
    if (!f.open(QIODevice::ReadOnly)) {
        return unreadable(LoadError::Kind::CannotRead, f.errorString());
    }
    // The path may have changed since status(). A character device (a tty,
    // /dev/input/*) opens at once, then blocks in read(), however little is
    // asked for: the bound below doesn't help.
    if (f.isSequential()) {
        return unreadable(LoadError::Kind::CannotRead, tr("not a regular file"));
    }
    // At most one byte more than the limit, never readAll(): the size is
    // judged on what was actually read. Checking size() first and then
    // reading all would let a file that grows in between (or a special file
    // that reports no size) be read without bound.
    const QByteArray json = f.read(max_store_file_size + 1);
    if (f.error() != QFileDevice::NoError) {
        return unreadable(LoadError::Kind::CannotRead, f.errorString());
    }
    if (json.size() > max_store_file_size) {
        return unreadable(LoadError::Kind::Damaged, tr("too large to be a programs file"));
    }
    auto content = fromJson(json);
    if (!content) {
        return unreadable(content.error().kind, content.error().message);
    }
    // m_nextId is at most max_program_id + 1, so this can't overflow. Out of
    // reach in practice (two billion programs added since the start), but an
    // id must never wrap around onto one in use.
    if (content->size() > max_program_id - m_nextId + 1) {
        return unreadable(LoadError::Kind::CannotRead,
                          tr("no program id is left: restart lpd8editor"));
    }
    replaceWith(*content);
    return {};
}

std::expected<void, QString> ProgramStore::save() const
{
    const QString file = QDir::toNativeSeparators(m_path);
    if (!isWritable()) {
        return Error(tr("%1 could not be read, so it is not overwritten: it may hold programs "
                        "that would be lost.")
                             .arg(file));
    }
    // load() refuses a file above max_store_file_size, and names have no
    // length limit: a file that large would lock the user out at the next
    // start (unreadable, so not overwritten either). Never write one.
    QList<NamedProgram> content;
    content.reserve(m_programs.size());
    for (const StoredProgram &stored : m_programs) {
        content.append(NamedProgram{.name = stored.name, .program = stored.program});
    }
    const QByteArray json = toJson(content);
    if (json.size() > max_store_file_size) {
        return Error(tr("The programs are too large to be saved in %1: a name is probably far "
                        "too long.")
                             .arg(file));
    }
    // On a first start, the application data folder doesn't exist yet.
    const QString folder = QFileInfo(m_path).absolutePath();
    if (!QDir().mkpath(folder)) {
        return Error(tr("Cannot create the folder %1.").arg(QDir::toNativeSeparators(folder)));
    }
    QSaveFile f(m_path);
    if (!f.open(QIODevice::WriteOnly)) {
        return Error(tr("Cannot write the programs file %1: %2").arg(file, f.errorString()));
    }
    f.write(json);
    if (!f.commit()) {
        return Error(tr("Cannot write the programs file %1: %2").arg(file, f.errorString()));
    }
    return {};
}

std::expected<QString, QString> ProgramStore::setAsideUnreadableFile(const QDateTime &now)
{
    if (!m_loadError) {
        return Error(tr("There is no unreadable programs file to set aside."));
    }
    // Only a file that was read and found damaged: renaming a directory, a
    // FIFO, a file this user can't read, or a newer format would move away
    // something that may be fine.
    if (m_loadError->kind != LoadError::Kind::Damaged) {
        return Error(tr("%1 is not a damaged programs file, so it is not set aside.")
                             .arg(QDir::toNativeSeparators(m_path)));
    }
    QString aside =
            m_path + QStringLiteral(".corrupt-") + now.toString(QStringLiteral("yyyyMMdd-hhmmss"));
    // QFile::rename() never replaces an existing file, so nothing can be lost
    // even if two set-asides happen in the same second (the second fails).
    if (!QFile::rename(m_path, aside)) {
        return Error(
                tr("Cannot rename %1 to %2.")
                        .arg(QDir::toNativeSeparators(m_path), QDir::toNativeSeparators(aside)));
    }
    m_loadError.reset();
    return aside;
}

} // namespace lpd8
