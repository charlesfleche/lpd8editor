#ifndef MIDIVALUEDELEGATE_H
#define MIDIVALUEDELEGATE_H

#include <QStyledItemDelegate>

class MidiValueDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    MidiValueDelegate(QObject* parent = Q_NULLPTR);

    QWidget* createEditor(QWidget*, const QStyleOptionViewItem&, const QModelIndex&) const Q_DECL_OVERRIDE;
    void setEditorData(QWidget*, const QModelIndex&) const Q_DECL_OVERRIDE;
    void setModelData(QWidget*, QAbstractItemModel* , const QModelIndex&) const Q_DECL_OVERRIDE;
    void updateEditorGeometry(QWidget*, const QStyleOptionViewItem&, const QModelIndex&) const Q_DECL_OVERRIDE;
};

#endif // MIDIVALUEDELEGATE_H
