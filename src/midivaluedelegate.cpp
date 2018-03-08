#include "midivaluedelegate.h"

#include <QSpinBox>

MidiValueDelegate::MidiValueDelegate(QObject* parent):
    QStyledItemDelegate(parent)
{

}

QWidget* MidiValueDelegate::createEditor(
        QWidget* parent,
        const QStyleOptionViewItem&,
        const QModelIndex&) const {
    Q_CHECK_PTR(parent);

    QSpinBox* editor = new QSpinBox(parent);
    editor->setFrame(false);
    editor->setMinimum(0);
    editor->setMaximum(127);
    return editor;
}

void MidiValueDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    Q_CHECK_PTR(editor);

    Q_CHECK_PTR(index.model());
    const int value = index.model()->data(index, Qt::EditRole).toInt();

    QSpinBox* spinBox = qobject_cast<QSpinBox*>(editor);
    Q_CHECK_PTR(spinBox);

    spinBox->setValue(value);
}

void MidiValueDelegate::setModelData(
        QWidget* editor,
        QAbstractItemModel* model,
        const QModelIndex& index) const {
    Q_CHECK_PTR(editor);
    Q_CHECK_PTR(model);

    QSpinBox* spinBox = qobject_cast<QSpinBox*>(editor);
    Q_CHECK_PTR(spinBox);

    spinBox->interpretText();
    const int value = spinBox->value();

    model->setData(index, value, Qt::EditRole);
}

void MidiValueDelegate::updateEditorGeometry(
        QWidget* editor,
        const QStyleOptionViewItem& option,
        const QModelIndex&) const {
    Q_CHECK_PTR(editor);

    editor->setGeometry(option.rect);
}
