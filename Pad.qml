import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

Control {
    id: root

    property var computedImplicitWidth: fontMetrics.boundingRect(noteText.text).width + grid.columnSpacing + noteSpinbox.implicitWidth
    property var computedImplicitHeight: title.implicitHeight + noteSpinbox.implicitHeight * 3 + buttonMomentary.implicitHeight + grid.rowSpacing * 4
    property var computedImplicitDimension: Math.max(computedImplicitWidth, computedImplicitHeight)

    implicitWidth: computedImplicitDimension
    implicitHeight: computedImplicitDimension

    Rectangle {
        anchors.fill: root
        color: "transparent"
        border.color: "red"
        border.width: 1
        z: 10
    }

    Rectangle {
        anchors.fill: grid
        color: "orange"
    }

    FontMetrics {
        id: fontMetrics
        font: noteText.font
    }

    GridLayout {
        id: grid

        anchors.fill: root
        columns: 2

        Text {
            id: title

            Layout.fillWidth: true
            Layout.columnSpan: 2

            text: presetId + " / " + programId + " / " + controlId
        }

        Text {
            id: noteText

            text: "Note"
        }

        SpinBox {
            id: noteSpinbox
            editable: true
            Layout.fillWidth: true;
            to: 127
            value: note
            Binding {
                target: model
                property: "note"
                value: noteSpinbox.value
            }
        }

        Text {
            id: notePC
            text: "PC"
        }
        SpinBox {
            id: pcSpinbox
            editable: true
            Layout.fillWidth: true;
            to: 127
            value: pc
            Binding {
                target: model
                property: "pc"
                value: pcSpinbox.value
            }
        }

        Text {
            id: noteCC
            text: "CC"
        }
        SpinBox {
            id: ccSpinbox
            editable: true
            Layout.fillWidth: true;
            to: 127
            value: cc
            Binding {
                target: model
                property: "cc"
                value: ccSpinbox.value
            }
        }

        Button {
            id: buttonMomentary

            Layout.fillWidth: true
            Layout.columnSpan: 2

            checkable: true
            text: "Momentary"
            onClicked: {
                model.momentary = checked
            }
        }
    }
}
