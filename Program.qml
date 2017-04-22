import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

Control {
    id: root

    implicitHeight: programName.implicitHeight
    implicitWidth: programName.implicitWidth + selectButton.implicitWidth + deleteButton.implicitWidth + row.spacing

    Rectangle {
        anchors.fill: root
        color: "orange"
    }

    RowLayout {
        id: row

        anchors.fill: root

        TextField {
            id: programName

            Layout.fillWidth: true

            readOnly: model.programId === 0

            text: model.name
            Binding {
                target: model
                property: "name"
                value: programName.text
            }
        }
        Button {
            id: selectButton
            text: "Select"
            onClicked: {
                app.activeProgramId = programId;
            }
        }
        Button {
            id: deleteButton
            enabled: model.programId > 0
            text: "Delete"
            onClicked: {
                app.deleteProgram(model.programId);
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
