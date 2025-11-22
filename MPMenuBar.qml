import QtQuick
import QtQuick.Controls

Rectangle {
    id: menubarRect
    width: parent.width
    height: 24
    color: "#0D1B3A"

    signal openTriggered()
    signal exitTriggered()

    Row {
        anchors.fill: parent
        spacing: 0

        Item {
            id: fileMenuButton
            width: 70
            height: menubarRect.height

            Rectangle {
                id: fileMenuRect
                anchors.fill: parent
                bottomRightRadius: 6
                color: fileMenuButtonMouseArea.containsMouse ? "#555" : "#444"
            }

            Text {
                anchors.centerIn: parent
                text: "File"
                color: "white"
            }

            MouseArea {
                id: fileMenuButtonMouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: fileMenu.open()
            }

            Menu {
                id: fileMenu
                x: fileMenuButton.x
                y: menubarRect.height

                MenuItem {
                    text: "Open Audio"
                    height: 20
                    onTriggered: menubarRect.openTriggered()
                }

                MenuItem {
                    text: "Exit"
                    height: 20
                    onTriggered: menubarRect.exitTriggered()
                }
            }
        }
    }
}
