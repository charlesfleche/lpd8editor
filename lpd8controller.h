#ifndef LPD8CONTROLLER_H
#define LPD8CONTROLLER_H

#include <QObject>

#include <asoundlib.h>

class Lpd8Controller : public QObject
{
    Q_OBJECT

public:
    explicit Lpd8Controller(QObject *parent = 0);

signals:

public slots:
private slots:
    void processEvent(snd_seq_event_t*);

private:
    void processClientStart(snd_seq_event_t*);

    bool m_connected;
};

#endif // LPD8CONTROLLER_H
