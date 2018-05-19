#ifndef MIDIVALUEDELEGATE_H
#define MIDIVALUEDELEGATE_H

#include <QSpinBox>
#include <QStyledItemDelegate>


class MidiValueDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    MidiValueDelegate(QObject* parent = nullptr);

    void paint(QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const override;
    QWidget* createEditor(QWidget*, const QStyleOptionViewItem&, const QModelIndex&) const override;
    void setEditorData(QWidget*, const QModelIndex&) const override;
    void setModelData(QWidget*, QAbstractItemModel* , const QModelIndex&) const override;
    void updateEditorGeometry(QWidget*, const QStyleOptionViewItem&, const QModelIndex&) const override;

    bool editorEvent(QEvent*, QAbstractItemModel*, const QStyleOptionViewItem&, const QModelIndex&) override;
};


class LutSpinBox : public QSpinBox {
    Q_OBJECT

public:
    LutSpinBox(QWidget *parent = nullptr);

    void setLut(const QStringList &lut);

protected:
    QString textFromValue(int value) const override;
    int valueFromText(const QString &text) const override;

private:
    QStringList m_lut;
};

QString lutTextFromValue(const QStringList &lut, int value);
int lutValueFromText(const QStringList &lut, const QString &text);

#endif // MIDIVALUEDELEGATE_H
