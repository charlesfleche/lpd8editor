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

            Repeater {
                model: 4
                delegate: ToolButton {
                    property int programId: modelData + 1

                    text: "↓" + programId
                    hoverEnabled: true

                    ToolTip.visible: hovered
                    ToolTip.text: "Get program " + programId + " from LPD8"
                    ToolTip.delay: 1000
                    ToolTip.timeout: 5000

                    onClicked: {
                        app.fetchProgram(programId)
                    }

                }
            }

            Repeater {
                model: 4
                delegate: ToolButton {
                    property int programId: modelData + 1

                    text: "↑" + programId
                    hoverEnabled: true

                    ToolTip.visible: down
                    ToolTip.text: "Send program " + programId + " to LPD8"
                    ToolTip.delay: 1000
                    ToolTip.timeout: 5000

                    onClicked: {
                        app.sendProgram(programId)
                    }
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }

    RowLayout {
        id: columns

        anchors.fill: parent

        ColumnLayout {
            id: presetsColumn

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumWidth: Screen.width / 8

            RowLayout {
                Text {
                    Layout.fillWidth: true

                    text: "Programs"
                }
                Button {
                    Layout.alignment: Qt.AlignRight
                    text: "Add"
                    onClicked: {
                        app.newProgram();
                    }
                }
            }

            ListView {
                id: presetsSection
                Layout.fillHeight: true
                Layout.fillWidth: true

                spacing: globalSpacing

                ScrollBar.vertical: ScrollBar{}

                model: programsModel
                delegate: Program {
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

            Text {
                text: "Pads"
            }

            GridView {
                id: padsView

                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.minimumHeight: padSize*2

                interactive: false

                cellHeight: padSize + globalSpacing
                cellWidth: padSize + globalSpacing

                verticalLayoutDirection: GridView.BottomToTop

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

                interactive: false

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
