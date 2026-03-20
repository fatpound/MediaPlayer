import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import FatPound.MediaPlayer 0.1

Window {
    id:      root
    width:   800
    height:  600
    visible: true
    title:   qsTr("Media Player")
    color:   MPTheme.colBgDeep

    MediaPlayerWrapper {
        id: mediaPlayer
    }

    FileDialog {
        id:          fileDialog
        title:       "Select Audio File"
        nameFilters: [ "Audio Files (*.mp3 *.m4a *.flac *.wav *.alac *.aiff)" ]

        onAccepted: {
            var path     = currentFile.toString()
            var fileName = decodeURIComponent(path.substring(path.lastIndexOf("/") + 1))

            mediaPlayer.loadAudio(path)
            root.title = fileName
        }
    }


    MPMenuBar {
        id:          mpMenuBar
        width:       parent.width
        anchors.top: parent.top

        onOpenTriggered: fileDialog.open()
        onExitTriggered: root.close()
    }


    MPSettingsButton {
        id:              settingsButton
        anchors.top:     parent.top
        anchors.right:   parent.right
        anchors.margins: 16

        onClicked_: {
            pitchControlPopup.x = settingsButton.x + settingsButton.width  / 2 - pitchControlPopup.width
            pitchControlPopup.y = settingsButton.y + settingsButton.height
            pitchControlPopup.open()
        }
    }


    MPPitchControlPopup {
        id: pitchControlPopup

        onPitchChanged: (value) => mediaPlayer.setPitch(value)
    }


    MPProgressBar {
        id:                       mpProgressBar
        anchors.bottom:           mpPlaybackControls.top
        anchors.bottomMargin:     15
        anchors.horizontalCenter: parent.horizontalCenter
        width:                    parent.width - 48

        progress: mediaPlayer.duration > 0 ? mediaPlayer.position / mediaPlayer.duration : 0.0
        position: mediaPlayer.position
        duration: mediaPlayer.duration

        onSeekRequested: (ratio) => {
            if (mediaPlayer.duration > 0)
                mediaPlayer.seek(mediaPlayer.duration * ratio)
            else
                mpProgressBar.seekCompleted()
        }
    }


    MPPlaybackControls {
        id:                       mpPlaybackControls
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom:           parent.bottom
        anchors.bottomMargin:     20
        isPlaying:                mediaPlayer.playing

        onPlayClicked:   mediaPlayer.play()
        onPauseClicked:  mediaPlayer.pause()
        onRewindClicked: mediaPlayer.fullRewind()
    }


    Connections {
        target: mediaPlayer

        function onSeekingChanged() {
            if (!mediaPlayer.seeking)
                mpProgressBar.seekCompleted()
        }
    }
}
