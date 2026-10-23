#include "midiconnectionsmodel.h"

#include "miditransport.h"

#include <algorithm>

using lpd8::MidiPort;
using lpd8::MidiTransport;

MidiConnectionsModel::MidiConnectionsModel(MidiTransport &transport, QObject *parent)
    : QAbstractListModel(parent)
    , m_transport(transport)
{
    for (const MidiPort &port : m_transport.ports()) {
        m_ids.append(port.id);
    }

    connect(&m_transport, &MidiTransport::portsChanged, this, &MidiConnectionsModel::updatePorts);
    connect(&m_transport,
            &MidiTransport::connectionChanged,
            this,
            &MidiConnectionsModel::notifyConnectionChanged);

    connect(this, &MidiConnectionsModel::dataChanged, this, [this]() {
        emit connectedPortChanged(connectedPort());
        emit connectedChanged(connected());
        emit externallyManagedChanged(externallyManaged());
    });
}

int MidiConnectionsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_ids.size()) + 1;
}

QVariant MidiConnectionsModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid) || role != Qt::DisplayRole) {
        return {};
    }
    if (index.row() == 0) {
        return tr("Disconnect");
    }
    const QString &id = m_ids.at(index.row() - 1);
    const QList<MidiPort> ports = m_transport.ports();
    const auto it = std::ranges::find(ports, id, &MidiPort::id);
    return it != ports.end() ? QVariant(it->name) : QVariant();
}

QModelIndex MidiConnectionsModel::connectedPort() const
{
    if (m_transport.externallyManaged()) {
        return {};
    }
    const std::optional<QString> id = m_transport.connectedPort();
    if (!id) {
        return index(0);
    }
    const qsizetype i = m_ids.indexOf(*id);
    return i >= 0 ? index(static_cast<int>(i) + 1) : QModelIndex();
}

bool MidiConnectionsModel::connected() const
{
    return m_transport.isConnected();
}

bool MidiConnectionsModel::externallyManaged() const
{
    return m_transport.externallyManaged();
}

void MidiConnectionsModel::connectPort(const QModelIndex &idx)
{
    Q_ASSERT(checkIndex(idx, CheckIndexOption::IndexIsValid));

    if (idx.row() == 0) {
        m_transport.disconnectAll();
    } else {
        m_transport.connectTo(m_ids.at(idx.row() - 1));
    }
    notifyConnectionChanged();
}

void MidiConnectionsModel::notifyConnectionChanged()
{
    // Every row's connection may have changed.
    emit dataChanged(index(0), index(rowCount() - 1));
}

void MidiConnectionsModel::updatePorts()
{
    QStringList now;
    for (const MidiPort &port : m_transport.ports()) {
        now.append(port.id);
    }

    // Ports gone: their rows are removed, last first.
    for (int i = static_cast<int>(m_ids.size()) - 1; i >= 0; --i) {
        if (!now.contains(m_ids.at(i))) {
            beginRemoveRows({}, i + 1, i + 1);
            m_ids.removeAt(i);
            endRemoveRows();
        }
    }
    // New ports: appended, in the transport's order.
    for (const QString &id : now) {
        if (!m_ids.contains(id)) {
            const int row = static_cast<int>(m_ids.size()) + 1;
            beginInsertRows({}, row, row);
            m_ids.append(id);
            endInsertRows();
        }
    }
    if (m_ids != now) {
        // The transport reordered its ports, which neither backend does:
        // start again rather than show them in another order.
        beginResetModel();
        m_ids = now;
        endResetModel();
    }
    // A port may have been renamed.
    notifyConnectionChanged();
}
