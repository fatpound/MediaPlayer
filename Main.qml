import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import Custom.MediaPlayer 1.0

Window {
    id: mainWindow
    width: 800
    height: 600
    visible: true
    title: qsTr("Media Player")
    color: "#0D1B3A"

    property string mediaFilePath: ""
    property string mediaFileName: "No Audio Loaded"

    MediaPlayerWrapper {
        id: mediaPlayer
    }


    FileDialog {
        id: fileAudioDialog
        title: "Select Audio File"
        nameFilters: [ "Audio Files (*.mp3 *.m4a *.flac *.wav *.alac *.aiff)" ]
        onAccepted: {
            mediaFilePath = currentFile.toString()
            var fullFileName = mediaFilePath.substring(mediaFilePath.lastIndexOf("/") + 1)
            var dotIndex = fullFileName.lastIndexOf(".")

            if (dotIndex !== -1) {
                mediaFileName = fullFileName.substring(0, dotIndex)
                mediaPlayer.loadAudio(mediaFilePath);
            }
        }
    }


    MPMenuBar {
        id: mpMenuBar
        width: parent.width
        anchors.top: parent.top

        onOpenTriggered: fileAudioDialog.open()
        onExitTriggered: mainWindow.close()
    }


    MPContentView {
        anchors.top: mpMenuBar.bottom
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width - 32
        anchors.margins: 16

        mediaFileName: mainWindow.mediaFileName
        progress: (mediaPlayer.duration > 0) ? mediaPlayer.position / mediaPlayer.duration : 0
        isPlaying: mediaPlayer.playing

        onSeekRequested: (ratio) => {
            mediaPlayer.seek(mediaPlayer.duration * ratio);
        }
        onRewindRequested: mediaPlayer.fullRewind()
        onPlayRequested: mediaPlayer.play()
        onPauseRequested: mediaPlayer.pause()
    }
}
