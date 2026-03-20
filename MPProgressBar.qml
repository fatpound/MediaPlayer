import QtQuick
import QtQuick.Controls
import FatPound.MediaPlayer 0.1

Item {
    id:     root
    width:  600
    height: MPTheme.progressBarHeight + 28

    property real progress: 0.0
    property int  position: 0
    property int  duration: 0

    property real _dragRatio:       0.0
    property real _displayProgress: _isDragging ? _dragRatio : progress
    property bool _isDragging:      false

    signal seekRequested(real ratio)

    function formatTime(ms) {
        var totalSec = Math.floor(ms / 1000)
        var minutes  = Math.floor(totalSec / 60)
        var seconds  = totalSec % 60

        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }

    function seekCompleted() {
        _isDragging = false
    }


    Text {
        id:                   positionLabel
        anchors.left:         parent.left
        anchors.bottom:       trackBar.top
        anchors.bottomMargin: 6
        color:                MPTheme.colTextSecondary
        font.pixelSize:       11
        text:                 root._isDragging ? root.formatTime(root.duration * root._dragRatio) : root.formatTime(root.position)
    }


    Text {
        id:                   durationLabel
        anchors.right:        parent.right
        anchors.bottom:       trackBar.top
        anchors.bottomMargin: 6
        text:                 root.formatTime(root.duration)
        color:                MPTheme.colTextSecondary
        font.pixelSize:       11
    }


    Item {
        id:                  trackBar
        anchors.bottom:      parent.bottom
        anchors.left:        parent.left
        anchors.right:       parent.right
        anchors.leftMargin:  0
        anchors.rightMargin: 0
        height:              MPTheme.progressBarHeight

        Rectangle {
            id:           backgroundRect
            anchors.fill: parent
            radius:       height / 2
            color:        MPTheme.colAccentDim
        }

        Rectangle {
            id:     fillRect
            width:  trackBar.width * (root._isDragging ? root._dragRatio : root.progress)
            height: parent.height
            radius: height / 2
            color:  MPTheme.colAccent
        }

        Rectangle {
            id:                     handleRect
            width:                  12
            height:                 12
            radius:                 6
            anchors.verticalCenter: parent.verticalCenter
            x:                      trackBar.width * (root._isDragging ? root._dragRatio : root.progress) - width / 2
            z:                      2
            color:                  "white"
            border.color:           MPTheme.colBorder
            border.width:           1
        }

        MouseArea {
            id:              seekArea
            anchors.fill:    parent
            anchors.margins: 0

            function ratioAt(mouseX) {
                return Math.max(0.0, Math.min(1.0, mouseX / trackBar.width))
            }

            onPressed:         (mouse) => {
                root._isDragging = true
                root._dragRatio  = ratioAt(mouse.x)
            }
            onPositionChanged: (mouse) => {
                if (pressed) root._dragRatio = ratioAt(mouse.x)
            }
            onReleased:        (mouse) => {
                root.seekRequested(root._dragRatio)
            }
        }
    }
}
