#ifndef MIDIVALUEDELEGATE_H
#define MIDIVALUEDELEGATE_H

#include <QSpinBox>
#include <QStyledItemDelegate>

class MidiValueDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    MidiValueDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor,
                      QAbstractItemModel *model,
                      const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override;

    bool editorEvent(QEvent *event,
                     QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;
};

class LutSpinBox : public QSpinBox
{
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
