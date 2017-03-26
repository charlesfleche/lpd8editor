import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0
import QtQuick.Window 2.0

ApplicationWindow {
    visible: true
    minimumWidth: Screen.width / 3
    minimumHeight: padsColumn.height
    title: qsTr("lpd8-editor") // XXX get applicationName

    property int globalSpacing: programButtons.spacing

//    MockupPadsModel {
//        id: padsModel
//    }

//    MockupKnobsModel {
//        id: knobsModel
//    }

    MockupProgramsModel {
        id: programsModel
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

            RowLayout {
                Text {
                    Layout.fillWidth: true;

                    text: "Presets"
                }
                Button {
                    Layout.alignment: Qt.AlignRight
                    text: "Add"
                    onClicked: {
                        app.newPreset();
                    }
                }
            }

            Rectangle {
                anchors.fill: presetsSection
                color: "darkblue"
            }

            ListView {
                id: presetsSection
                Layout.fillHeight: true
                Layout.fillWidth: true

                spacing: globalSpacing

                ScrollBar.vertical: ScrollBar{}

                interactive: true

                model: presets
                delegate: Preset {
                    width: parent.width
                }
            }
        }

        GridLayout {
            id: padsColumn

            Layout.fillHeight: true
            Layout.fillWidth: true

            Layout.alignment: Qt.AlignTop

            columns: 4

            Rectangle {
                anchors.fill: padsColumn
                color: "lightgreen"
            }

            Text {
                text: "Pads"
            }

            RowLayout {
                id: programButtons

                Layout.columnSpan: 3

                Layout.fillWidth: false
                Layout.alignment: Qt.AlignRight

                Repeater {
                    model: programsModel
                    delegate: Button {
                        Layout.fillWidth: true

                        autoExclusive: true
                        checkable: true

                        checked: model.current
                        text: "Program " + (model.programId)
                        onClicked: {
                            app.activeProgramId = model.programId
                        }
                    }
                }
            }

            Repeater {
                model: padsModel
                delegate: Pad {
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                }
                onCountChanged: {
                    console.log('Pads repeater count: ' + count)
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
        }
    }
}
