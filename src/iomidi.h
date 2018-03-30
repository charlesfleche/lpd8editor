#ifndef IOMIDI_H
#define IOMIDI_H

#include <QObject>

class IOMidi : public QObject
{
    Q_OBJECT
public:
    explicit IOMidi(QObject *parent = nullptr);

signals:

public slots:
};

#endif // IOMIDI_H