import QtQuick
import QtQuick.Controls

Item {
    id: progressBarItem
    width: 600
    height: 10

    property real progress: 0.0

    signal seekRequested(real ratio)

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: "#3A4F73"
    }

    Rectangle {
        width: progressBarItem.width * progressBarItem.progress
        height: parent.height
        radius: height / 2
        color: "#70C1FF"
    }

    Rectangle {
        width: 12
        height: 12
        radius: 6
        anchors.verticalCenter: parent.verticalCenter
        x: progressBarItem.width * progressBarItem.progress - width / 2
        color: "white"
        border.color: "#888"
        border.width: 1
        z: 2
    }

    MouseArea {
            anchors.fill: parent

            onPressed: (mouse) => {
                let ratio = mouse.x / progressBarItem.width
                ratio = Math.max(0, Math.min(1, ratio)) // 0–1 arası kısıtla
                progressBarItem.progress = ratio
                seekRequested(ratio)
            }

            onPositionChanged: (mouse) => {
                if (pressed) {
                    let ratio = mouse.x / progressBarItem.width
                    ratio = Math.max(0, Math.min(1, ratio))
                    progressBarItem.progress = ratio
                    seekRequested(ratio)
                }
            }
        }
}
