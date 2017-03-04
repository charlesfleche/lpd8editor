import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

Item {
    id: root

    implicitHeight: presetName.implicitHeight

    Rectangle {
        anchors.fill: root
        color: "orange"
    }

    RowLayout {
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
}
