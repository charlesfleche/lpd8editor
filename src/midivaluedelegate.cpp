#include "midivaluedelegate.h"

#include "enums.h"

#include <QApplication>
#include <QPushButton>
#include <QSpinBox>

#include <QtDebug>

QWidget* createDefaultEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) {
    QSpinBox* ret = new QSpinBox(parent);
    ret->setFrame(false);
    ret->setMinimum(0);
    ret->setMaximum(127);
    return ret;
}

QWidget* createToggleEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) {
    QPushButton* ret = new QPushButton(parent);
    ret->setCheckable(true);
    return ret;
}

void setDefaultEditorData(QWidget* editor, const QModelIndex& index) {
    Q_CHECK_PTR(editor);

    QSpinBox* spinBox = qobject_cast<QSpinBox*>(editor);
    Q_CHECK_PTR(spinBox);

    Q_CHECK_PTR(index.model());
    const int value = index.model()->data(index, Qt::EditRole).toInt();

    spinBox->setValue(value);
}

void setToggleEditorData(QWidget* editor, const QModelIndex& index) {
    Q_CHECK_PTR(editor);

    QPushButton* button = qobject_cast<QPushButton*>(editor);
    Q_CHECK_PTR(button);

    Q_CHECK_PTR(index.model());
    const bool value = index.model()->data(index, Qt::EditRole).toBool();

    button->setChecked(value);
}

void setDefaultModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) {
    Q_CHECK_PTR(editor);
    Q_CHECK_PTR(model);

    QSpinBox* spinBox = qobject_cast<QSpinBox*>(editor);
    Q_CHECK_PTR(spinBox);

    spinBox->interpretText();
    model->setData(index, spinBox->value(), Qt::EditRole);
}

void setToggleModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) {
    Q_CHECK_PTR(editor);
    Q_CHECK_PTR(model);

    QPushButton* button = qobject_cast<QPushButton*>(editor);
    Q_CHECK_PTR(button);

    model->setData(index, button->isChecked(), Qt::EditRole);
}

void paintToggle(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) {
    Q_CHECK_PTR(painter);

    QStyleOptionButton buttonOption;
    buttonOption.rect = option.rect;
    buttonOption.state = option.state;

    const bool isToggle = index.data().toBool();
    buttonOption.state |= isToggle ? QStyle::State_On : QStyle::State_Off;
    buttonOption.text = isToggle ? "Toggle" : "Momentary";
    QApplication* app = qobject_cast<QApplication*>(qGuiApp);
    Q_CHECK_PTR(app);
    app->style()->drawControl(QStyle::CE_PushButton, &buttonOption, painter);
}

MidiValueDelegate::MidiValueDelegate(QObject* parent):
    QStyledItemDelegate(parent)
{

}

void MidiValueDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Q_CHECK_PTR(index.model());

    switch (index.model()->data(index, MidiDataRole::MidiValueType).toInt()) {
    case MidiType::DefaultType:
    case MidiType::NoteType:
        QStyledItemDelegate::paint(painter, option, index);
        break;
    case MidiType::ToggleType:
        paintToggle(painter, option, index);
        break;
    }
}

QWidget* MidiValueDelegate::createEditor(
        QWidget* parent,
        const QStyleOptionViewItem& style,
        const QModelIndex& index) const {
    Q_CHECK_PTR(index.model());

    switch (index.model()->data(index, MidiDataRole::MidiValueType).toInt()) {
    case MidiType::DefaultType:
    case MidiType::NoteType:
        return createDefaultEditor(parent, style, index);
    case MidiType::ToggleType:
        return Q_NULLPTR;
//        return createToggleEditor(parent, style, index);
    }

    Q_ASSERT(false); // Should never reach this point
    return Q_NULLPTR;
}

void MidiValueDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    Q_CHECK_PTR(index.model());

    switch (index.model()->data(index, MidiDataRole::MidiValueType).toInt()) {
    case MidiType::DefaultType:
    case MidiType::NoteType:
        setDefaultEditorData(editor, index);
        break;
    case MidiType::ToggleType:
        setToggleEditorData(editor, index);
        break;
    }
}

void MidiValueDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
    Q_CHECK_PTR(editor);
    Q_CHECK_PTR(model);

    switch (index.model()->data(index, MidiDataRole::MidiValueType).toInt()) {
    case MidiType::DefaultType:
    case MidiType::NoteType:
        setDefaultModelData(editor, model, index);
        break;
    case MidiType::ToggleType:
        setToggleModelData(editor, model, index);
        break;
    }
}

void MidiValueDelegate::updateEditorGeometry(
        QWidget* editor,
        const QStyleOptionViewItem& option,
        const QModelIndex&) const {
    Q_CHECK_PTR(editor);

    editor->setGeometry(option.rect);
}

bool MidiValueDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) {
    Q_CHECK_PTR(event);
    Q_CHECK_PTR(model);

    if (model->data(index, MidiDataRole::MidiValueType) != MidiType::ToggleType) {
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
        model->setData(index, !model->data(index).toBool());
        return true;
    }
    return false;
}
