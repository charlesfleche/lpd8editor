#include "midiportsmodel.h"

#include <QtDebug>

MidiPortsModel::MidiPortsModel(snd_seq_t* seq_handle, int seq_port, QObject *parent)
    : QAbstractListModel(parent),
      m_seq_handle(seq_handle),
      m_seq_port(seq_port)
{
    Q_CHECK_PTR(m_seq_handle);
    Q_ASSERT(m_seq_port >= 0);
}

int MidiPortsModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return m_port_addrs.count();
}

QVariant MidiPortsModel::data(const QModelIndex &index, int role) const
{
    Q_ASSERT(index.row() < rowCount());

    if (!index.isValid())
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
        const int r = index.row();
        const snd_seq_addr_t addr = m_port_addrs[r];
        snd_seq_port_info_t* pinfo;
        snd_seq_port_info_alloca(&pinfo);
        Q_CHECK_PTR(m_seq_handle);
        if (snd_seq_get_any_port_info(m_seq_handle, addr.client, addr.port, pinfo) < 0) {
            qDebug() << "Cannot get port info for client" << addr.client << addr.port;
            return QVariant();
        }

//        snd_seq_port_info_set_addr(pinfo, addr);
//        snd_seq_port_info_t* pinfo = m_ports[r];
        const char* name = snd_seq_port_info_get_name(pinfo);
        return QString(name);
    }
    return QVariant();
}

//bool MidiPortsModel::insertRows(int row, int count, const QModelIndex &parent)
//{
//    beginInsertRows(parent, row, row + count - 1);
//    // FIXME: Implement me!
//    endInsertRows();
//}

//bool MidiPortsModel::removeRows(int row, int count, const QModelIndex &parent)
//{
//    beginRemoveRows(parent, row, row + count - 1);
//    // FIXME: Implement me!
//    endRemoveRows();
//}

void MidiPortsModel::rescanPorts() {
    Q_CHECK_PTR(m_seq_handle);

    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;

    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);
    snd_seq_client_info_set_client(cinfo, -1);
    int count = -1;
    while (snd_seq_query_next_client(m_seq_handle, cinfo) >= 0) {
        const int id = snd_seq_client_info_get_client(cinfo);
        snd_seq_port_info_set_client(pinfo, id);
        snd_seq_port_info_set_port(pinfo, -1);

        while (snd_seq_query_next_port(m_seq_handle, pinfo) >= 0) {
            beginInsertRows(QModelIndex(), 0, 0);
            m_port_addrs.append(*snd_seq_port_info_get_addr(pinfo));
            ++count;
//            qDebug() << count << snd_seq_port_info_get_name(pinfo);
            endInsertRows();
            qDebug() << id
//                     << snd_seq_client_info_get_client(cinfo)
                     << snd_seq_client_info_get_name(cinfo)
                     << (snd_seq_client_info_get_type(cinfo) == SND_SEQ_USER_CLIENT ? "user" : "kernel")
                     << "|"
                     << snd_seq_port_info_get_port(pinfo)
                     << snd_seq_port_info_get_name(pinfo)
                     << m_port_addrs[count].client
                     << m_port_addrs[count].port;
        }
    }

//    for (int i = 0 ; i < m_ports.count() ; ++i) {
//        qDebug() << i << count << snd_seq_port_info_get_name(m_ports[i]);
//    }
    // XXX crashes ?
//    snd_seq_port_info_free(pinfo);
//    snd_seq_client_info_free(cinfo);
}

void MidiPortsModel::connect(const QModelIndex & index) const {
    Q_ASSERT(index.row() < rowCount());
    Q_CHECK_PTR(m_seq_handle);
    Q_ASSERT(m_seq_port >= 0);

    const int r = index.row();
    const snd_seq_addr_t addr = m_port_addrs[r];
    snd_seq_port_info_t* pinfo;
    snd_seq_port_info_alloca(&pinfo);

    if (snd_seq_connect_from(m_seq_handle, m_seq_port, addr.client, addr.port) < 0) {
        qDebug() << "Cannot connect from";
    }

    if (snd_seq_connect_to(m_seq_handle, m_seq_port, addr.client, addr.port) < 0) {
        qDebug() << "Cannot connect to";
    }

//    snd_seq_port_subscribe_t *subs;
//    snd_seq_port_subscribe_alloca(&subs);
//    snd_seq_port_subscribe_set_sender(subs, &sender);
//    snd_seq_port_subscribe_set_dest(subs, &dest);
//    snd_seq_port_subscribe_set_queue(subs, queue);
//    snd_seq_port_subscribe_set_exclusive(subs, exclusive);
//    snd_seq_port_subscribe_set_time_update(subs, convert_time);
//    snd_seq_port_subscribe_set_time_real(subs, convert_real);

//    if (snd_seq_get_port_subscription(seq, subs) == 0) {
//          snd_seq_close(seq);
//          fprintf(stderr, "Connection is already subscribed\n");
//          return 1;
//    }
//    if (snd_seq_subscribe_port(seq, subs) < 0) {
//          snd_seq_close(seq);
//          fprintf(stderr, "Connection failed (%s)\n", snd_strerror(errno));
//          return 1;
//    }
}
