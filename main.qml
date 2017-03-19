import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

ApplicationWindow {
    visible: true
    width: 1280
    height: 720
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
    }

//    Rectangle {
//        anchors.fill: columns
//        color: "darkgrey"
//    }

    Rectangle {
        anchors.fill: widgetTester
        color: "darkgrey"
    }

//    ListView {
//        id: widgetTester
//        anchors.fill: parent
//        spacing: 10
//        model: padsModel
//        delegate: Pad {
////            width: parent.width
//        }
//    }

    GridView {
        id: widgetTester
        anchors.fill: parent
//        spacing: 10
        cellHeight: contentItem.children[0].implicitHeight
        cellWidth: contentItem.children[0].implicitWidth
        model: padsModel
        delegate: Pad {
            width: widgetTester.cellWidth
//            height: widgetTester.cellHeight
//            width: parent.width
        }
    }

/*
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

        ColumnLayout {
            id: padsColumn

            Layout.fillHeight: true
            Layout.fillWidth: true

            Rectangle {
                anchors.fill: padsColumn
                color: "lightblue"
            }

            ListView {
                Layout.fillHeight: true

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
                spacing: 8
                model: knobsModel
                delegate: Knob {
                    width: parent.width
                }
            }
        }
    }
*/
}
