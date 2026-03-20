import QtQuick
import QtQuick.Controls
import FatPound.MediaPlayer 0.1

Row {
    id:      root
    spacing: 10

    property bool isPlaying: false

    signal rewindClicked()
    signal playClicked()
    signal pauseClicked()


    ToolButton {
        width:      MPTheme.controlButtonSize
        height:     MPTheme.controlButtonSize
        background: null
        onClicked:  root.rewindClicked()

        contentItem: Image {
            source:       "qrc:/assets/rewind.png"
            anchors.fill: parent
            fillMode:     Image.PreserveAspectFit
        }
    }

    ToolButton {
        width:      MPTheme.controlButtonSize
        height:     MPTheme.controlButtonSize
        background: null
        onClicked:  root.isPlaying ? root.pauseClicked() : root.playClicked()

        contentItem: Image {
            source:       root.isPlaying ? "qrc:/assets/pause.png" : "qrc:/assets/play.png"
            anchors.fill: parent
            fillMode:     Image.PreserveAspectFit
        }
    }
}
