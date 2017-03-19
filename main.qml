import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

ApplicationWindow {
    visible: true
    width: 1280
    height: 720
    title: qsTr("lpd8-editor") // XXX get applicationName

    property int globalSpacing: programButtons.spacing

    MockupPadsModel {
        id: padsModel
    }

    MockupKnobsModel {
        id: knobsModel
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
