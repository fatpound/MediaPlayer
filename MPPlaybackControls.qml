import QtQuick
import QtQuick.Controls

Row {
    id: controlRoot
    spacing: 10

    property bool isPlaying: false

    signal rewindClicked()
    signal playClicked()
    signal pauseClicked()


    ToolButton {
        id: fullRewindButton
        width: 48
        height: 48
        background: null
        onClicked: controlRoot.rewindClicked()

        contentItem: Image {
            source: "qrc:/assets/rewind.png"
            anchors.fill: parent
            fillMode: Image.PreserveAspectFit
        }
    }


    ToolButton {
        id: playPauseButton
        width: 48
        height: 48
        background: null

        onClicked: isPlaying ? controlRoot.pauseClicked() : controlRoot.playClicked()

        contentItem: Image {
            source: isPlaying ? "qrc:/assets/pause.png" : "qrc:/assets/play.png"
            anchors.fill: parent
            fillMode: Image.PreserveAspectFit
        }
    }
}
