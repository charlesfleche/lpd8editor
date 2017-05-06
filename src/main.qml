import QtQuick 2.7
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0

import Qt.labs.platform 1.0

ApplicationWindow {
    visible: true
    minimumWidth: (presetsColumn.Layout.minimumWidth + padsColumn.Layout.minimumWidth) * 1.1 // 1.1 temp hack
    minimumHeight: toolBar.height + padsColumn.height
    title: qsTr("lpd8-editor") // XXX get applicationName

    property int globalSpacing: 14
    property int padSize: 256
    property int knobHeight: 256

    header: ToolBar {
        id: toolBar
        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: "New"
                onClicked: {
                    app.newProgram();
                }
            }

            ToolButton {
                enabled: app.activeProgramId > 0
                text: "Delete"
                onClicked: {
                    app.deleteProgram(app.activeProgramId);
                }
            }

            ToolSeparator {}

            Text {
                text: "Get"
            }

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

            ToolSeparator {}

            Text {
                text: "Send"
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

            ToolSeparator {}

            ToolButton {
                text: "Import"
                onClicked: {
                    fileDialogImport.open();
                }
            }

            ToolButton {
                text: "Export"
                onClicked: {
                    fileDialogExport.open();
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }

    FileDialog {
        id: fileDialogImport
        title: "Import a LPD8 preset"
        acceptLabel: 'Import'
        fileMode: FileDialog.OpenFiles
        folder: StandardPaths.writableLocation(StandardPaths.HomeLocation)

        onAccepted: {
            files.map(function f(url) {
                app.importProgram(url);
            })
        }
    }

    FileDialog {
        id: fileDialogExport
        title: "Export a LPD8 preset"
        acceptLabel: 'Export'
        fileMode: FileDialog.SaveFile
        folder: StandardPaths.writableLocation(StandardPaths.HomeLocation)

        onAccepted: app.exportActiveProgram(file);
    }

    RowLayout {
        id: columns

        anchors.fill: parent

        ColumnLayout {
            id: presetsColumn

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumWidth: Screen.width / 8

            Text {
                text: "Programs"
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
                color: "lightgrey"
                opacity: .5
            }

            GridView {
                id: padsView

                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.minimumHeight: (padSize + globalSpacing) * 2

                interactive: false

                cellHeight: padSize + globalSpacing
                cellWidth: padSize + globalSpacing

                verticalLayoutDirection: GridView.BottomToTop

                model: padsModel
                delegate: Control {
                    padding: globalSpacing
                    width: GridView.view.cellWidth
                    height: GridView.view.cellHeight
                    Rectangle {
                        color: "lightgrey"
                        width: padSize
                        height: padSize
                        anchors.centerIn: parent
                    }

                    Pad {
                        anchors.centerIn: parent
                    }
                }
            }

            GridView {
                id: knobsView

                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.minimumHeight: (padSize + globalSpacing) * 2

                interactive: false

                cellHeight: padSize + globalSpacing
                cellWidth: padSize + globalSpacing

                model: knobsModel
                delegate: Control {
                    padding: globalSpacing
                    width: GridView.view.cellWidth
                    height: GridView.view.cellHeight
                    Rectangle {
                        color: "lightgrey"
                        width: padSize
                        height: padSize
                        anchors.centerIn: parent
                    }

                    Knob {
                        anchors.centerIn: parent
                    }
                }
            }
        }
    }
}
