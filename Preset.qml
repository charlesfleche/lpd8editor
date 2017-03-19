import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

Control {
    id: root

    implicitHeight: presetName.implicitHeight
    implicitWidth: presetName.implicitWidth + selectButton.implicitWidth + deleteButton.implicitWidth + row.spacing

    Rectangle {
        anchors.fill: root
        color: "orange"
    }

    RowLayout {
        id: row

        anchors.fill: root

        TextField {
            id: presetName

            Layout.fillWidth: true

            text: name
            Binding {
                target: model
                property: "name"
                value: presetName.text
            }
        }
        Button {
            id: selectButton
            text: "Select"

        }

        Button {
            id: deleteButton
            text: "Delete"
            onClicked: {
                app.deletePreset(presetId);
            }
        }
    }

    Rectangle {
        anchors.fill: root
        color: "transparent"
        border.color: "red"
        border.width: 1
    }
}
