#include "midivaluedelegate.h"

#include "enums.h"

#include <QApplication>
#include <QPushButton>
#include <QSpinBox>

#include <QtDebug>

MidiType midiType(const QModelIndex& index) {
    return static_cast<MidiType>(index.data(MidiDataRole::MidiValueType).toInt());
}

bool hasLut(const QModelIndex &index) {
    return index.data(MidiDataRole::MidiValues).isValid();
}

QStringList lut(const QModelIndex &index) {
    Q_ASSERT(hasLut(index));

    return index.data(MidiDataRole::MidiValues).toStringList();
}

bool isToggle(const QModelIndex &index) {
    return hasLut(index) && lut(index).size() == 2;
}

QWidget* createSpinboxEditor(QWidget* parent, int min, int max) {
    QSpinBox* ret = new QSpinBox(parent);
    ret->setFrame(false);
    ret->setMinimum(min);
    ret->setMaximum(max);
    return ret;
}

void setDefaultEditorData(QWidget* editor, const QModelIndex& index) {
    Q_CHECK_PTR(editor);

    QSpinBox* spinBox = qobject_cast<QSpinBox*>(editor);
    Q_CHECK_PTR(spinBox);

    Q_CHECK_PTR(index.model());
    const int value = index.model()->data(index).toInt();

    spinBox->setValue(value);
}

void setDefaultModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) {
    Q_CHECK_PTR(editor);
    Q_CHECK_PTR(model);

    QSpinBox* spinBox = qobject_cast<QSpinBox*>(editor);
    Q_CHECK_PTR(spinBox);

    spinBox->interpretText();
    model->setData(index, spinBox->value(), Qt::EditRole);
}

void paintToggle(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) {
    Q_CHECK_PTR(painter);

    QStyleOptionButton buttonOption;
    buttonOption.rect = option.rect;
    buttonOption.state = option.state;

    buttonOption.state |= index.data(Qt::EditRole).toInt() == 0 ? QStyle::State_Off : QStyle::State_On;
    buttonOption.text = index.data().toString();
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

    isToggle(index) ? paintToggle(painter, option, index) : QStyledItemDelegate::paint(painter, option, index);
}

QWidget* MidiValueDelegate::createEditor(
        QWidget* /*parent*/,
        const QStyleOptionViewItem&,
        const QModelIndex& /*index*/) const {
//    Q_CHECK_PTR(index.model());

    QWidget* ret = Q_NULLPTR;

//    switch (midiType(index)) {
//    case MidiType::ChannelType:
//    case MidiType::DefaultType:
//    case MidiType::NoteType:
//        ret = createSpinboxEditor(
//                  parent,
//                  index.data(MidiDataRole::MidiValueMin).toInt(),
//                  index.data(MidiDataRole::MidiValueMax).toInt()
//        );
//        break;
//    default:
//        ret = Q_NULLPTR;
//        break;
//    }

    return ret;
}

void MidiValueDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    Q_CHECK_PTR(index.model());

    switch (midiType(index)) {
    case MidiType::ChannelType:
    case MidiType::DefaultType:
    case MidiType::NoteType:
        setDefaultEditorData(editor, index);
        break;
    default:
        break;
    }
}

void MidiValueDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
    Q_CHECK_PTR(editor);
    Q_CHECK_PTR(model);

    switch (midiType(index)) {
    case MidiType::ChannelType:
    case MidiType::DefaultType:
    case MidiType::NoteType:
        setDefaultModelData(editor, model, index);
        break;
    default:
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

    const bool toggleClicked = isToggle(index) && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick);
    if (toggleClicked) {
        const int currentValue = model->data(index, Qt::EditRole).toInt();
        const int newValue = currentValue == 0 ? 1 : 0;
        model->setData(index, newValue);
        return true;
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
