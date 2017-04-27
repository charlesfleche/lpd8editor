import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

Control {
    id: root

    implicitHeight: programName.implicitHeight
    implicitWidth: programName.implicitWidth + deleteButton.implicitWidth + row.spacing

    Rectangle {
        anchors.fill: root
        color: index % 2 == 0 ? "transparent" : "lightgrey"
        opacity: .5
    }

    RowLayout {
        id: row

        anchors.fill: root

        Text {
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter
            opacity: programId == app.activeProgramId ? 1 : 0;
            text: "•"
        }

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

        Text {
            horizontalAlignment: Text.AlignRight
            Layout.fillWidth: true;
            text: "Channel"
        }
        SpinBox {
            id: channelSpinbox
            editable: true
            from: 1
            to: 16
            value: channel
            Binding {
                target: model
                property: "channel"
                value: channelSpinbox.value
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

    MouseArea {
        anchors.fill: parent
        propagateComposedEvents: true
        preventStealing: false
        onPressed: {
            app.activeProgramId = programId;
            mouse.accepted = false;
        }
    }
}
