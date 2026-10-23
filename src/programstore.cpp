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
#include <initializer_list>
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

QJsonObject programToJson(const StoredProgram &stored)
{
    QJsonArray pads;
    for (const Pad &pad : stored.program.pads) {
        pads.append(padToJson(pad));
    }
    QJsonArray knobs;
    for (const Knob &knob : stored.program.knobs) {
        knobs.append(knobToJson(knob));
    }
    return {{"id", stored.id},
            {"name", stored.name},
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

std::expected<StoredProgram, QString> programFromJson(const QJsonObject &object)
{
    StoredProgram stored;

    // id + 1 must fit in an int: see max_program_id.
    const auto id = wholeNumber(object.value("id"_L1));
    if (!id || *id < 1 || *id > max_program_id) {
        return Error(tr("'id' must be an integer from 1 to %1").arg(max_program_id));
    }
    stored.id = static_cast<int>(*id);

    const QJsonValue name = object.value("name"_L1);
    if (!name.isString()) {
        return Error(tr("program %1: 'name' must be a string").arg(stored.id));
    }
    stored.name = name.toString();

    // From here, errors name the program.
    const auto inProgram = [&stored](const QString &error) {
        return Error(tr("program %1 (%2): %3").arg(QString::number(stored.id), stored.name, error));
    };

    if (const auto unknown =
                unknownKey(object, {"id"_L1, "name"_L1, "channel"_L1, "pads"_L1, "knobs"_L1})) {
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

QByteArray toJson(const StoreContent &content)
{
    QJsonArray array;
    for (const StoredProgram &stored : content.programs) {
        array.append(programToJson(stored));
    }
    const QJsonObject root{
            {"format", store_format_version}, {"nextId", content.nextId}, {"programs", array}};
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

std::expected<StoreContent, QString> fromJson(const QByteArray &json)
{
    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return Error(tr("not valid JSON (%1 at offset %2)")
                             .arg(parseError.errorString(), QString::number(parseError.offset)));
    }
    if (!document.isObject()) {
        return Error(tr("not an lpd8editor programs file"));
    }
    const QJsonObject root = document.object();

    const QJsonValue formatValue = root.value("format"_L1);
    if (!formatValue.isDouble()) {
        return Error(tr("not an lpd8editor programs file (no format version)"));
    }
    // Compared as a number: a version that is not an integer, or doesn't fit
    // in an int, must not be read as 0.
    const double format = formatValue.toDouble();
    if (format > store_format_version) {
        // Never half-read a newer format: saving it back would lose what this
        // version doesn't know about.
        return Error(tr("written by a newer version of lpd8editor (format %1, this version reads "
                        "up to %2)")
                             .arg(QString::number(format), QString::number(store_format_version)));
    }
    if (format != store_format_version) {
        return Error(tr("unknown format version %1").arg(QString::number(format)));
    }
    if (const auto unknown = unknownKey(root, {"format"_L1, "nextId"_L1, "programs"_L1})) {
        return Error(*unknown);
    }

    // nextId may be max_program_id + 1: then no id is left to hand out.
    const auto nextId = wholeNumber(root.value("nextId"_L1));
    if (!nextId || *nextId < 1 || *nextId > max_program_id + 1.0) {
        return Error(tr("'nextId' must be an integer from 1 to %1").arg(max_program_id + 1));
    }

    const QJsonValue programsValue = root.value("programs"_L1);
    if (!programsValue.isArray()) {
        return Error(tr("'programs' must be a list"));
    }

    QList<StoredProgram> programs;
    for (const QJsonValue value : programsValue.toArray()) {
        if (!value.isObject()) {
            return Error(tr("each program must be an object"));
        }
        auto stored = programFromJson(value.toObject());
        if (!stored) {
            return Error(stored.error());
        }
        const bool duplicate = std::ranges::any_of(
                programs, [&](const StoredProgram &p) { return p.id == stored->id; });
        if (duplicate) {
            return Error(tr("two programs have the id %1").arg(stored->id));
        }
        programs.append(std::move(*stored));
    }

    // Never hand out an id in use, even if the file was edited by hand. Every
    // id is at most max_program_id, so 'p.id + 1' can't overflow.
    const int next = std::ranges::fold_left(
            programs, static_cast<int>(*nextId), [](int n, const StoredProgram &p) {
                return std::max(n, p.id + 1);
            });
    return StoreContent{.programs = std::move(programs), .nextId = next};
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

std::optional<int> ProgramStore::add(const QString &name, const Program &program, int id)
{
    if (!program.isValid() || !isAcceptableName(name)) {
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

    const auto row = static_cast<int>(m_programs.size());
    emit programAboutToBeAdded(id, row);
    m_programs.append(StoredProgram{.id = id, .name = name, .program = program});
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

std::expected<void, QString> ProgramStore::load()
{
    const QString file = QDir::toNativeSeparators(m_path);
    const auto replaceWith = [this](StoreContent content) {
        emit programsAboutToBeReset();
        m_programs = std::move(content.programs);
        // Also above any id handed out earlier in this session.
        m_nextId = std::max(m_nextId, content.nextId);
        m_loadError.clear();
        emit programsReset();
    };

    QFile f(m_path);
    if (!f.exists()) {
        // The first start: no programs yet. An empty store stays as it is,
        // without signals: a change that changes nothing emits nothing.
        m_loadError.clear();
        if (!m_programs.isEmpty()) {
            replaceWith(StoreContent{.programs = {}, .nextId = 1});
        }
        return {};
    }
    const auto unreadable = [this, &file](const QString &why) {
        m_loadError = tr("Cannot read the programs file %1: %2").arg(file, why);
        return Error(m_loadError);
    };
    if (f.size() > max_store_file_size) {
        return unreadable(tr("too large to be a programs file"));
    }
    if (!f.open(QIODevice::ReadOnly)) {
        return unreadable(f.errorString());
    }
    auto content = fromJson(f.readAll());
    if (!content) {
        return unreadable(content.error());
    }
    replaceWith(std::move(*content));
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
    // On a first start, the application data folder doesn't exist yet.
    const QString folder = QFileInfo(m_path).absolutePath();
    if (!QDir().mkpath(folder)) {
        return Error(tr("Cannot create the folder %1.").arg(QDir::toNativeSeparators(folder)));
    }
    QSaveFile f(m_path);
    if (!f.open(QIODevice::WriteOnly)) {
        return Error(tr("Cannot write the programs file %1: %2").arg(file, f.errorString()));
    }
    f.write(toJson(StoreContent{.programs = m_programs, .nextId = m_nextId}));
    if (!f.commit()) {
        return Error(tr("Cannot write the programs file %1: %2").arg(file, f.errorString()));
    }
    return {};
}

std::expected<QString, QString> ProgramStore::setAsideUnreadableFile()
{
    if (isWritable()) {
        return Error(tr("There is no unreadable programs file to set aside."));
    }
    QString aside = m_path + QStringLiteral(".corrupt-")
            + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-hhmmss"));
    // QFile::rename() never replaces an existing file, so nothing can be lost
    // even if two set-asides happen in the same second (the second fails).
    if (!QFile::rename(m_path, aside)) {
        return Error(
                tr("Cannot rename %1 to %2.")
                        .arg(QDir::toNativeSeparators(m_path), QDir::toNativeSeparators(aside)));
    }
    m_loadError.clear();
    return aside;
}

} // namespace lpd8
