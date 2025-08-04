import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Custom.MediaPlayer 1.0

Window {
    id: mainWindow
    width: 800
    height: 600
    visible: true
    title: qsTr("Media Player by fatpound - Copyright (c) 2025")
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
            console.log("Selected file URL: " + mediaFilePath)

            var fullFileName = mediaFilePath.substring(mediaFilePath.lastIndexOf("/") + 1)
            console.log("Selected file name: " + fullFileName)

            var dotIndex = fullFileName.lastIndexOf(".")

            if (dotIndex !== -1)
            {
                mediaFileName = fullFileName.substring(0, dotIndex)
                mediaPlayer.loadAudio(mediaFilePath);
            }
        }
        onRejected: {
            console.log("File dialog canceled")
        }
    }


    Rectangle
    {
        id: menubarRect
        width: parent.width
        height: 24
        anchors.top: parent.top
        color: mainWindow.color

        Row
        {
            anchors.fill: parent
            spacing: 0

            Item
            {
                id: fileMenuButton
                width: 70
                height: menubarRect.height

                Rectangle
                {
                    id: fileMenuRect
                    anchors.fill: parent
                    bottomRightRadius: 6
                    color: fileMenuButtonMouseArea.containsMouse ? "#555" : "#444"
                }

                Text
                {
                    anchors.centerIn: parent
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "File"
                    color: "white"
                }

                MouseArea
                {
                    id: fileMenuButtonMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        fileMenu.open()
                    }
                }

                Menu
                {
                    id: fileMenu
                    x: fileMenuButton.x
                    y: menubarRect.height

                    MenuItem
                    {
                        text: "Open Audio"
                        height: 20
                        onTriggered: {
                            fileAudioDialog.open()
                        }
                    }

                    MenuItem
                    {
                        text: "Exit"
                        height: 20
                        onTriggered: {
                            mainWindow.close()
                        }
                    }
                }
            }
        }
    }


    Rectangle {
        property int margin: 16

        id: centerBox
        width: parent.width - margin * 2
        height: parent.height - margin * 2 - menubarRect.height
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: menubarRect.bottom
        anchors.margins: margin
        radius: 12
        color: "#1B263B"
        border.color: "#415A77"
        border.width: 1

        Text {
            id: fileNameText
            text: mediaFileName
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
            anchors.bottom: playbackButtonsRow.top
            anchors.bottomMargin: playbackButtonsRow.margin
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - parent.margin * 4

            progress: (mediaPlayer.duration > 0) ? mediaPlayer.position / mediaPlayer.duration : 0

            onSeekRequested: (ratio) => {
                mediaPlayer.seek(mediaPlayer.duration * ratio);
            }
        }

        //------------------//
        // Playback Buttons //
        //------------------//

        Row {
            property int margin: 15

            id: playbackButtonsRow
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: margin
            spacing: 10

            ToolButton {
                id: fullRewindButton
                width: 48
                height: 48
                background: null
                onClicked: {
                    mediaPlayer.fullRewind();

                }

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

                onClicked: {
                    mediaPlayer.playing ? mediaPlayer.pause() : mediaPlayer.play();
                }

                contentItem: Image {
                    source: mediaPlayer.playing ? "qrc:/assets/pause.png" : "qrc:/assets/play.png"
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectFit
                }
            }
        }
    }
}
