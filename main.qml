import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0
import QtQuick.Window 2.0

ApplicationWindow {
    visible: true
    minimumWidth: (presetsColumn.Layout.minimumWidth + padsColumn.Layout.minimumWidth) * 1.1 // 1.1 temp hack
    minimumHeight: Screen.height / 4
    title: qsTr("lpd8-editor") // XXX get applicationName

    property int globalSpacing: programButtons.spacing
    property alias padSize: invisiblePad.computedImplicitDimension

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

    Pad {
        id: invisiblePad
        visible: false
    }

    RowLayout {
        id: columns

        anchors.fill: parent

        ColumnLayout {
            id: presetsColumn

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumWidth: Screen.width / 8

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
            Layout.fillWidth: false

            Layout.alignment: Qt.AlignTop

            columns: 4

            Layout.minimumWidth: 4 * (padSize+globalSpacing)

            Rectangle {
                anchors.fill: padsColumn
                color: "lightgreen"
            }

            Text {
                Layout.columnSpan: 3
                Layout.fillWidth: true
                text: "Program"
            }

            RowLayout {
                id: programButtons

                Layout.columnSpan: 1

                Layout.fillWidth: false
                Layout.alignment: Qt.AlignRight

                Repeater {
                    model: programsModel
                    delegate: Button {
                        Layout.fillWidth: true

                        autoExclusive: true
                        checkable: true

                        checked: model.current
                        text: model.programId
                        onClicked: {
                            app.activeProgramId = model.programId
                        }
                    }
                }
            }

            Text {
                text: "Pads"
                Layout.columnSpan: 4
            }

            GridView {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Layout.columnSpan: 4
                Layout.rowSpan: 2

                cellHeight: padSize + globalSpacing
                cellWidth: padSize + globalSpacing

                model: padsModel
                delegate: Item {
                    width: GridView.view.cellWidth
                    height: GridView.view.cellHeight
                    Pad {
                        anchors.centerIn: parent
                    }
                }
            }

            Text {
                text: "Knobs"
                Layout.columnSpan: 4
            }
/*
            GridView {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Layout.columnSpan: 4
                Layout.rowSpan: 2

                cellHeight: padSize + globalSpacing
                cellWidth: padSize + globalSpacing

                model: knobsModel
                delegate: Item {
                    width: GridView.view.cellWidth
                    height: GridView.view.cellHeight
                    Knob {
                        anchors.centerIn: parent
                    }
                }
            }
*/
            Item {
                Layout.columnSpan: 4
                Layout.fillHeight: true
                Layout.fillWidth: true
            }

            /*
            Repeater {
                model: padsModel
                delegate: Pad {
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                }
            }
            */
            /*
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
            */
        }
    }
}
