import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

ApplicationWindow {
    visible: true
    width: 640
    height: 480
    title: qsTr("lpd8-editor") // XXX get applicationName

    ListModel {
        id: padsModel
        ListElement {
            presetId: 0
            programId: 0
            controlId: 1
            note: 1
            pc: 2
            cc: 3
            momentary: 0
        }
        ListElement {
            presetId: 0
            programId: 0
            controlId: 2
            note: 4
            pc: 5
            cc: 6
            momentary: 0
        }
        ListElement {
            presetId: 0
            programId: 0
            controlId: 3
            note: 7
            pc: 8
            cc: 9
            momentary: 0
        }
    }

    ListModel {
        id: knobsModel
        ListElement {
            presetId: 0
            programId: 0
            controlId: 1
            cc: 10
        }
        ListElement {
            presetId: 0
            programId: 0
            controlId: 2
            cc: 11
        }
        ListElement {
            presetId: 0
            programId: 0
            controlId: 3
            cc: 12
        }
    }

    Rectangle {
        anchors.fill: columns
        color: "chocolate"
    }

    RowLayout {
        id: columns

        anchors.fill: parent

        ColumnLayout {
            id: presetsColumn

            Rectangle {
                anchors.fill: presetsColumn
                color: "steelblue"
            }

            Rectangle {
                anchors.fill: presetsSection
                color: "lavenderblush"
            }

            Text {
                text: "Presets"
            }

            ListView {
                id: presetsSection
                Layout.fillHeight: true
                Layout.fillWidth: true

                interactive: false

                model: presets
                delegate: Preset {
                    width: parent.width
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

        ColumnLayout {
            id: padsColumn

            Layout.fillHeight: true

            Rectangle {
                anchors.fill: padsColumn
                color: "lightblue"
            }

            Text {
                text: "Pads"
            }

            ListView {
                Layout.fillHeight: true
                Layout.fillWidth: true
                spacing: 8
                model: padsModel
                delegate: Pad {
                    width: parent.width
                }
            }
        }

        ColumnLayout {
            id: knobsColumn

            Layout.fillHeight: true

            Rectangle {
                anchors.fill: knobsColumn
                color: "lightcoral"
            }

            Text {
                text: "Knobs"
            }

            ListView {
                Layout.fillHeight: true
                Layout.fillWidth: true
                spacing: 8
                model: knobsModel
                delegate: Knob {
                    width: parent.width
                }
            }
        }
    }
}
