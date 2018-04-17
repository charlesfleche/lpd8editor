#include "midiconnectionsmodel.h"

#include "iomidi.h"

#include <QtDebug>

bool operator==(const snd_seq_addr_t& lhs, const snd_seq_addr_t& rhs) {
    return lhs.client == rhs.client && lhs.port == rhs.port;
}

MidiConnectionsModel::MidiConnectionsModel(IOMidi* io) :
    QAbstractListModel(io),
    m_io(io)
{
    Q_CHECK_PTR(m_io);

    connect(
        m_io,
        &IOMidi::eventReceived,
        this,
        &MidiConnectionsModel::handleMidiEvent);

    connect(
        this,
        &MidiConnectionsModel::dataChanged,
        [=]() {
            emit connectedPortChanged(connectedPort());
            emit connectedChanged(connected());
            emit externallyManagedChanged(externallyManaged());
        }
    );

    m_addrs.append({0, 0}); // Row for disconnect choice

    // Loop from already connected clients and ports to populate the addresses list

    snd_seq_client_info_t *cinfo;
    snd_seq_client_info_alloca(&cinfo);

    snd_seq_port_info_t *pinfo;
    snd_seq_port_info_alloca(&pinfo);

    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(m_io->handle(), cinfo) >= 0) {
        if (!acceptClient(cinfo)) {
            continue;
        }

        snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(m_io->handle(), pinfo) >= 0) {
            if (!acceptPort(pinfo)) {
                continue;
            }

            const snd_seq_addr_t* addr = snd_seq_port_info_get_addr(pinfo);
            Q_CHECK_PTR(addr);

            addAddress(addr);
        }
    }
}

int MidiConnectionsModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_addrs.count();
}

QVariant MidiConnectionsModel::data(const QModelIndex &index, int role) const {
    if (index.row() == 0) {
        if (role == Qt::DisplayRole) {
            return tr("Disconnect");
        }
    }

    if (role == Qt::DisplayRole) {
        return portName(index);
    }
    return QVariant();
}

QModelIndex MidiConnectionsModel::connectedPort() const {
    Q_ASSERT(m_addrs.count() > 0);

    QModelIndex ret = index(0);
    for (int r = 1 ; r < m_addrs.count() ; ++r) {
        switch (portConnection(m_addrs[r])) {
        case Direction::Disconnected:
            break;

        // Partially connected, done externally
        case Direction::From:
        case Direction::To:
            return QModelIndex();

        // If a second addr is connected both ways, done externally
        case Direction::BothWays:
            if (ret.row() != 0) {
                return QModelIndex();
            }
            ret = index(r);
            break;
        }
    }
    return ret;
}

bool MidiConnectionsModel::connected() const {
    for (int r = 0 ; r < m_addrs.count() ; ++r) {
        if (portConnection(m_addrs[r]) > Direction::Disconnected) {
            return true;
        }
    }
    return false;
}

bool MidiConnectionsModel::externallyManaged() const {
    return connected() && !connectedPort().isValid();
}

void MidiConnectionsModel::connectPort(const QModelIndex &idx) {
    Q_ASSERT(idx.isValid());

    disconnectAllPorts();

    if (idx.row() > 0) {
        connectPort(m_addrs[idx.row()]);
    }

    emit dataChanged(index(0), index(m_addrs.count()));
}

void MidiConnectionsModel::disconnectAllPorts() const {
    Q_CHECK_PTR(m_io);

    snd_seq_query_subscribe_t *subs;
    snd_seq_query_subscribe_alloca(&subs);
    const snd_seq_addr_t ownaddr = {m_io->clientId(), m_io->portId()};
    snd_seq_query_subscribe_set_root(subs, &ownaddr);

    snd_seq_addr_t addr;

    snd_seq_query_subscribe_set_index(subs, 0);
    snd_seq_query_subscribe_set_type(subs, SND_SEQ_QUERY_SUBS_READ);
    while (snd_seq_query_port_subscribers(m_io->handle(), subs) >= 0) {
        addr = *snd_seq_query_subscribe_get_addr(subs);
        if (snd_seq_disconnect_to(m_io->handle(), m_io->portId(), addr.client, addr.port) < 0) {
            qWarning() << "Cannot disconnect to" << addr.client << addr.port;

            // This happens when a2j is running: the index is never incremented
            // by snd_seq_query_port_subscribers, so it is manually incremented here
            const int idx = snd_seq_query_subscribe_get_index(subs) + 1;
            snd_seq_query_subscribe_set_index(subs, idx);
        }
    }
    snd_seq_query_subscribe_set_index(subs, 0);
    snd_seq_query_subscribe_set_type(subs, SND_SEQ_QUERY_SUBS_WRITE);
    while (snd_seq_query_port_subscribers(m_io->handle(), subs) >= 0) {
        addr = *snd_seq_query_subscribe_get_addr(subs);
        if (snd_seq_disconnect_from(m_io->handle(), m_io->portId(), addr.client, addr.port) < 0) {
            qWarning() << "Cannot disconnect from" << addr.client << addr.port;

            // This happens when a2j is running: the index is never incremented
            // by snd_seq_query_port_subscribers, so it is manually incremented here
            const int idx = snd_seq_query_subscribe_get_index(subs) + 1;
            snd_seq_query_subscribe_set_index(subs, idx);
        }
    }
}

void MidiConnectionsModel::connectPort(const snd_seq_addr_t &addr) {
    Q_CHECK_PTR(m_io);

    if (snd_seq_connect_from(m_io->handle(), m_io->portId(), addr.client, addr.port) < 0) {
        qWarning() << "Cannot connect from";
    }

    if (snd_seq_connect_to(m_io->handle(), m_io->portId(), addr.client, addr.port) < 0) {
        qWarning() << "Cannot connect to";
    }
}

void MidiConnectionsModel::handleMidiEvent(const snd_seq_event_t* ev) {
    Q_CHECK_PTR(m_io);
    Q_CHECK_PTR(ev);

    const snd_seq_addr_t ownaddr = {m_io->clientId(), m_io->portId()};

    switch (ev->type) {
    case SND_SEQ_EVENT_PORT_START:
        return addAddressIfAccepted(&ev->data.addr);
    case SND_SEQ_EVENT_PORT_EXIT:
        return removeAddressIfAccepted(&ev->data.addr);
    case SND_SEQ_EVENT_PORT_CHANGE:
        return notifyDataChangedForAddress(&ev->data.addr);
    case SND_SEQ_EVENT_PORT_SUBSCRIBED:
    case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
        if (ownaddr == ev->data.connect.sender || ownaddr == ev->data.connect.dest) {
            if (ownaddr == ev->data.connect.sender) {
                return notifyDataChangedForAddress(&ev->data.connect.dest);
            }
            if (ownaddr == ev->data.connect.dest) {
                return notifyDataChangedForAddress(&ev->data.connect.sender);
            }
        }
        return;
    case SND_SEQ_EVENT_CLIENT_START:
    case SND_SEQ_EVENT_CLIENT_EXIT:
        emit connectedPortChanged(connectedPort());
        return;
    default:
        return;
    }
}

void MidiConnectionsModel::addAddressIfAccepted(const snd_seq_addr_t *addr) {
    Q_CHECK_PTR(addr);

    if (acceptAddress(addr)) {
        addAddress(addr);
    }
}

void MidiConnectionsModel::removeAddressIfAccepted(const snd_seq_addr_t *addr) {
    Q_CHECK_PTR(addr);

    const int r = m_addrs.indexOf(*addr);
    if (r >= 0) {
        beginRemoveRows(QModelIndex(), r, r);
        m_addrs.removeAt(r);
        endRemoveRows();
    }
}

void MidiConnectionsModel::notifyDataChangedForAddress(const snd_seq_addr_t *addr) {
    Q_CHECK_PTR(addr);

    const int r = m_addrs.indexOf(*addr);
    if (r >= 0) {
        const QModelIndex idx = index(r);
        emit dataChanged(idx, idx);
    }
}

void MidiConnectionsModel::addAddress(const snd_seq_addr_t *addr) {
    Q_CHECK_PTR(addr);
    Q_ASSERT(!m_addrs.contains(*addr));

    beginInsertRows(QModelIndex(), m_addrs.count(), m_addrs.count());
    m_addrs.append(*addr);
    endInsertRows();
}

bool MidiConnectionsModel::acceptAddress(const snd_seq_addr_t *addr) const {
    Q_CHECK_PTR(addr);

    snd_seq_client_info_t *cinfo;
    snd_seq_client_info_alloca(&cinfo);
    if (snd_seq_get_any_client_info(m_io->handle(), addr->client, cinfo) < 0 ||
        !acceptClient(cinfo)) {
        return false;
    }

    snd_seq_port_info_t *pinfo;
    snd_seq_port_info_alloca(&pinfo);
    return snd_seq_get_any_port_info(m_io->handle(), addr->client, addr->port, pinfo) == 0 &&
           acceptPort(pinfo);
}

bool MidiConnectionsModel::acceptClient(const snd_seq_client_info_t *info) const {
    Q_CHECK_PTR(m_io);
    Q_CHECK_PTR(info);

    const int id = snd_seq_client_info_get_client(info);
    return id != m_io->clientId() && id != SND_SEQ_CLIENT_SYSTEM;
}

bool MidiConnectionsModel::acceptPort(const snd_seq_port_info_t *info) const {
    Q_CHECK_PTR(info);

    const unsigned int caps = snd_seq_port_info_get_capability(info);
    return !(caps & SND_SEQ_PORT_CAP_NO_EXPORT) &&
           caps & SND_SEQ_PORT_CAP_READ &&
           caps & SND_SEQ_PORT_CAP_WRITE;
}

QString MidiConnectionsModel::portName(const QModelIndex& index) const {
    Q_ASSERT(index.isValid());

    const snd_seq_addr_t &addr = m_addrs[index.row()];
    snd_seq_port_info_t *info;
    snd_seq_port_info_alloca(&info);
    const int res = snd_seq_get_any_port_info(m_io->handle(), addr.client, addr.port, info);
    Q_UNUSED(res);
    Q_ASSERT(res >= 0);
    return snd_seq_port_info_get_name(info);
}

MidiConnectionsModel::Directions MidiConnectionsModel::portConnection(const QModelIndex& index) const {
    Q_CHECK_PTR(m_io);
    Q_ASSERT(index.isValid());

    return portConnection(m_addrs[index.row()]);
}

MidiConnectionsModel::Directions MidiConnectionsModel::portConnection(const snd_seq_addr_t& addr) const {
    Q_CHECK_PTR(m_io);

    MidiConnectionsModel::Directions ret = Direction::Disconnected;

    const snd_seq_addr_t ownaddr = {m_io->clientId(), m_io->portId()};

    snd_seq_port_subscribe_t *subs;
    snd_seq_port_subscribe_alloca(&subs);

    snd_seq_port_subscribe_set_sender(subs, &ownaddr);
    snd_seq_port_subscribe_set_dest(subs, &addr);
    if (snd_seq_get_port_subscription(m_io->handle(), subs) == 0) {
        ret |= Direction::To;
    }

    snd_seq_port_subscribe_set_sender(subs, &addr);
    snd_seq_port_subscribe_set_dest(subs, &ownaddr);
    if (snd_seq_get_port_subscription(m_io->handle(), subs) == 0) {
        ret |= Direction::From;
    }

    return ret;
}
