import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

ApplicationWindow {
    visible: true
    width: 640
    height: 480
    title: qsTr("lpd8-editor") // XXX get applicationName

    ListView {
        anchors.fill: parent
        model: presets
        delegate: Text {
            text: 'Preset: ' + name
        }
    }
}
