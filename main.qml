import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0
import QtQuick.Window 2.0

ApplicationWindow {
    visible: true
    minimumWidth: (presetsColumn.Layout.minimumWidth + padsColumn.Layout.minimumWidth) * 1.1 // 1.1 temp hack
    minimumHeight: padsColumn.height
    title: qsTr("lpd8-editor") // XXX get applicationName

    property int globalSpacing: 0
    property int padSize: 256
    property int knobHeight: 256

    header: ToolBar {
        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: "Get"
                hoverEnabled: true

                ToolTip.visible: hovered
                ToolTip.text: "Get programs from LPD8"
                ToolTip.delay: 1000
                ToolTip.timeout: 5000

                onClicked: {
                    app.fetchPrograms()
                }

            }
            ToolButton {
                text: "Send"
                hoverEnabled: true

                ToolTip.visible: down
                ToolTip.text: "Send programs to LPD8"
                ToolTip.delay: 1000
                ToolTip.timeout: 5000
            }
            Item {
                Layout.fillWidth: true
            }
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

        ColumnLayout {
            id: padsColumn

            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.minimumWidth: 4 * (padSize+globalSpacing)
            Layout.alignment: Qt.AlignTop

            Rectangle {
                anchors.fill: padsColumn
                color: "lightgreen"
            }

            RowLayout {
                Text {
                    Layout.fillWidth: true
                    text: "Program"
                }

                RowLayout {
                    id: programButtons

                    Layout.fillWidth: false
                    Layout.alignment: Qt.AlignRight

                    Repeater {
                        model: 4
                        delegate: Button {
                            property int programId: modelData + 1
                            Layout.fillWidth: true

                            autoExclusive: true
                            checkable: true

                            checked: programId === app.activeProgramId
                            text: programId
                            onClicked: {
                                app.activeProgramId = programId
                            }
                        }
                    }
                }
            }

            Text {
                text: "Pads"
            }

            GridView {
                id: padsView

                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.minimumHeight: padSize*2

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
            }

            GridView {
                id: knobsView

                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.minimumHeight: padSize*2

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
        }
    }
}
