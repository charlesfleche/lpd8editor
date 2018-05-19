#include "midivaluedelegate.h"

#include "enums.h"

#include <QApplication>

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

bool isSpinbox(const QModelIndex &index) {
    return hasLut(index) && lut(index).size() > 2;
}

MidiValueDelegate::MidiValueDelegate(QObject* parent):
    QStyledItemDelegate(parent)
{

}

void MidiValueDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (isToggle(index)) {
        QStyleOptionButton buttonOption;
        buttonOption.rect = option.rect;
        buttonOption.state = option.state;

        buttonOption.state |= index.data(Qt::EditRole).toInt() == 0 ? QStyle::State_Off : QStyle::State_On;
        buttonOption.text = index.data().toString();
        QApplication* app = qobject_cast<QApplication*>(qGuiApp);
        Q_CHECK_PTR(app);
        app->style()->drawControl(QStyle::CE_PushButton, &buttonOption, painter);
        return;
    }

    QStyledItemDelegate::paint(painter, option, index);
}

QWidget* MidiValueDelegate::createEditor(
        QWidget* parent,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const {
    if (isSpinbox(index)) {
        return new LutSpinBox(parent);
    }

    return QStyledItemDelegate::createEditor(parent, option, index);
}

void MidiValueDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    if (isSpinbox(index)) {
        LutSpinBox *sb = qobject_cast<LutSpinBox*>(editor);
        Q_CHECK_PTR(sb);

        sb->setLut(lut(index));
        sb->setValue(index.data(Qt::EditRole).toInt());
        return;
    }

    QStyledItemDelegate::setEditorData(editor, index);
}

void MidiValueDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
    if (isSpinbox(index)) {
        LutSpinBox *sb = qobject_cast<LutSpinBox*>(editor);
        Q_CHECK_PTR(sb);

        Q_CHECK_PTR(model);
        model->setData(index, sb->value(), Qt::EditRole);
        return;
    }

    QStyledItemDelegate::setModelData(editor, model, index);
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


LutSpinBox::LutSpinBox(QWidget *parent) :
    QSpinBox(parent)
{

}

void LutSpinBox::setLut(const QStringList &lut) {
    m_lut = lut;
    setMinimum(0);
    setMaximum(m_lut.count() - 1);
}

QString LutSpinBox::textFromValue(int value) const {
    return lutTextFromValue(m_lut, value);
}

int LutSpinBox::valueFromText(const QString &text) const {
    return lutValueFromText(m_lut, text);
}

QString lutTextFromValue(const QStringList &lut, int value) {
    return lut.isEmpty() ? QString() : lut.at(value);
}

int lutValueFromText(const QStringList &lut, const QString &text) {
    return lut.indexOf(text);
}
