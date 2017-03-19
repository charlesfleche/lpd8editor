import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

Control {
    id: root

    property var computedImplicitWidth: fontMetrics.boundingRect(noteText.text).width + grid.columnSpacing + mySpinBox.implicitWidth
    property var computedImplicitHeight: title.implicitHeight + mySpinBox.implicitHeight * 3 + buttonMomentary.implicitHeight + grid.rowSpacing * 4
    property var computedImplicitDimentsion: Math.max(computedImplicitWidth, computedImplicitHeight)

    implicitWidth: computedImplicitWidth
    implicitHeight: computedImplicitHeight

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
//        rowSpacing: 0
//        columnSpacing: 0
        Text {
            id: title

            Layout.columnSpan: 2

            text: presetId + " / " + programId + " / " + controlId
        }

        Text {
            id: noteText

            text: "Note"
        }

        SpinBox {
            id: mySpinBox
            editable: true
            Layout.fillWidth: true;
            to: 127
            value: 127
        }

        Text {
            id: notePC
            text: "PC"
        }
        SpinBox {
            Layout.fillWidth: true;
        }

        Text {
            id: noteCC
            text: "CC"
        }
        SpinBox {
            Layout.fillWidth: true;
        }

        Button {
            id: buttonMomentary

            Layout.fillWidth: true
            Layout.columnSpan: 2

            checkable: true
            text: "Momentary"
        }
    }
}
