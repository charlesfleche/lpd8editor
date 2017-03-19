import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

Item {
    id: root

    height: childrenRect.height
    width: height
//    width: childrenRect.width

    Rectangle {
        anchors.fill: root
        color: "orange"
    }

    FontMetrics {
        id: fontMetrics
        font: mySpinBox.font
    }

    ColumnLayout {
        width: root.width

        Text {
            id: title

            Layout.fillWidth: true

            text: presetId + " / " + programId + " / " + controlId
        }
        GridLayout {
            id: gridValues

            Layout.fillHeight: true
            Layout.fillWidth: true

            columns: 2

            Text {
                id: noteText
                text: "Note"
            }
            SpinBox {
                id: mySpinBox
                editable: true
                Layout.fillWidth: true;
                Layout.minimumWidth: up.indicator.implicitWidth + down.indicator.implicitWidth + fontMetrics.maximumCharacterWidth * 3
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
        }
        Button {
            id: buttonMomentary
            Layout.fillWidth: true

            checkable: true
            text: "Momentary"
        }
    }
}
