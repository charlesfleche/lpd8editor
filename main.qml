import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

ApplicationWindow {
    visible: true
    width: 1280
    height: 720
    title: qsTr("lpd8-editor") // XXX get applicationName

    property int globalSpacing: programButtons.spacing

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
        ListElement {
            presetId: 0
            programId: 0
            controlId: 4
            note: 10
            pc: 11
            cc: 12
            momentary: 0
        }
        ListElement {
            presetId: 0
            programId: 0
            controlId: 5
            note: 13
            pc: 14
            cc: 15
            momentary: 0
        }
        ListElement {
            presetId: 0
            programId: 0
            controlId: 6
            note: 16
            pc: 17
            cc: 18
            momentary: 0
        }
        ListElement {
            presetId: 0
            programId: 0
            controlId: 7
            note: 19
            pc: 20
            cc: 21
            momentary: 0
        }
        ListElement {
            presetId: 0
            programId: 0
            controlId: 8
            note: 22
            pc: 23
            cc: 24
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
        ListElement {
            presetId: 0
            programId: 0
            controlId: 4
            cc: 13
        }
        ListElement {
            presetId: 0
            programId: 0
            controlId: 5
            cc: 14
        }
        ListElement {
            presetId: 0
            programId: 0
            controlId: 6
            cc: 15
        }
        ListElement {
            presetId: 0
            programId: 0
            controlId: 7
            cc: 16
        }
        ListElement {
            presetId: 0
            programId: 0
            controlId: 8
            cc: 17
        }
    }

    Rectangle {
        anchors.fill: columns
        color: "darkgrey"
    }

    RowLayout {
        id: columns

        anchors.fill: parent

        ColumnLayout {
            id: presetsColumn

            Layout.fillHeight: true
            Layout.fillWidth: true

            Rectangle {
                anchors.fill: presetsColumn
                color: "steelblue"
            }

            Text {
                text: "Presets"
            }

            Rectangle {
                anchors.fill: presetsSection
                color: "darkblue"
            }

            ListView {
                id: presetsSection
                Layout.fillHeight: true
                Layout.fillWidth: true

                interactive: false

                spacing: globalSpacing

                model: presets
                delegate: Preset {
                    width: parent.width
                }
            }

            RowLayout {
                id: programButtons

                Layout.fillWidth: true

                Button {
                    Layout.fillWidth: true

                    checkable: true
                    text: "1"
                }
                Button {
                    Layout.fillWidth: true

                    checkable: true
                    text: "2"
                }
                Button {
                    Layout.fillWidth: true

                    checkable: true
                    text: "3"
                }
                Button {
                    Layout.fillWidth: true

                    checkable: true
                    text: "4"
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

        GridLayout {
            id: padsColumn

            Layout.fillHeight: true
            Layout.fillWidth: true

            columns: 4

            Rectangle {
                anchors.fill: padsColumn
                color: "lightgreen"
            }

            Text {
                Layout.columnSpan: 4

                text: "Pads"
            }

            Repeater {
                model: padsModel
                delegate: Pad {
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                }
            }

            Text {
                Layout.columnSpan: 4

                text: "Knobs"
            }

            Repeater {
                model: knobsModel
                delegate: Knob {
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}
