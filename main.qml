import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

ApplicationWindow {
    visible: true
    width: 640
    height: 480
    title: qsTr("lpd8-editor") // XXX get applicationName

    ColumnLayout {
        anchors.fill: parent
        ListView {
            Layout.fillHeight: true
            model: presets
            delegate: RowLayout {
                TextField {
                    text: name
                    onAccepted: {
                        app.setPresetName(presetId, text);
                    }
                }
                Button {
                    text: "Delete"
                    onClicked: {
                        app.deletePreset(presetId);
                    }
                }
            }
        }
        Button {
            Layout.fillWidth: true
            text: "Add preset"
            onClicked: {
                app.newPreset();
            }
        }
    }
}
