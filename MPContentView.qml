import QtQuick
import QtQuick.Controls

Rectangle {
    id: contentRoot

    property int margin: 16

    property string mediaFileName: "No Audio Loaded"
    property real progress: 0.0
    property bool isPlaying: false

    signal seekRequested(real ratio)
    signal rewindRequested()
    signal playRequested()
    signal pauseRequested()

    radius: 12
    color: "#1B263B"
    border.color: "#415A77"
    border.width: 1

    Text {
        id: fileNameText
        text: contentRoot.mediaFileName
        color: "#E0E1DD"
        font.pixelSize: 22
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 12
        anchors.topMargin: 8
        opacity: 0.45
    }

    MPProgressBar {
        id: mpProgressBar
        anchors.bottom: mpPlaybackButtons.top
        anchors.bottomMargin: 15
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width - parent.margin * 4
        progress: contentRoot.progress

        onSeekRequested: (ratio) => contentRoot.seekRequested(ratio)
    }

    MPPlaybackControls {
        id: mpPlaybackButtons
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 15
        isPlaying: contentRoot.isPlaying

        onRewindClicked: contentRoot.rewindRequested()
        onPlayClicked:   contentRoot.playRequested()
        onPauseClicked:  contentRoot.pauseRequested()
    }

    MPPitchControlPopup {
        id: pitchControlPopup
    }

    ToolButton {
        id: settingsButton
        anchors.top: contentRoot.top
        anchors.right: contentRoot.right
        anchors.margins: 12
        width: 32
        height: 32

        contentItem: Image {
            id: tune_effects_img
            source: "qrc:/assets/tune_effects.png"
            sourceSize.width: 24
            sourceSize.height: 24
            fillMode: Image.PreserveAspectFit
            opacity: 0.7
        }

        onClicked: {
            pitchControlPopup.x = settingsButton.x + settingsButton.width  / 2 - tune_effects_img.width  / 2 - pitchControlPopup.width
            pitchControlPopup.y = settingsButton.y + settingsButton.height / 2 + tune_effects_img.height / 2
            pitchControlPopup.open()
        }
    }
}
