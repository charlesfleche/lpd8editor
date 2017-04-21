import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

Control {
    id: root

    property var computedImplicitWidth: fontMetrics.boundingRect(highText.text).width + grid.columnSpacing + ccSpinbox.implicitWidth
    property var computedImplicitHeight: title.implicitHeight + ccSpinbox.implicitHeight * 3 + grid.rowSpacing * 3
//    property var computedImplicitDimentsion: Math.max(computedImplicitWidth, computedImplicitHeight)

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
        font: ccText.font
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
            id: ccText
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

        Text {
            id: lowText
            text: "Low"
        }
        SpinBox {
            id: lowSpinbox
            editable: true
            Layout.fillWidth: true;
            to: 127
            value: low
            Binding {
                target: model
                property: "low"
                value: lowSpinbox.value
            }
        }

        Text {
            id: highText
            text: "High"
        }
        SpinBox {
            id: highSpinbox
            editable: true
            Layout.fillWidth: true;
            to: 127
            value: high
            Binding {
                target: model
                property: "high"
                value: highSpinbox.value
            }
        }
    }
}
