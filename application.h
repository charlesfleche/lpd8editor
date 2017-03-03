#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>

class QAbstractItemModel;

class Application : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* presets READ presets CONSTANT)

public:
    explicit Application(QObject *parent = 0);

    QAbstractItemModel* presets() const;

private:
    QAbstractItemModel* m_presets;
};

#endif // APPLICATION_H
