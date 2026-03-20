import QtQuick
import QtQuick.Controls
import FatPound.MediaPlayer 0.1

Rectangle {
    id:     root
    width:  parent.width
    height: MPTheme.menuBarHeight
    color:  MPTheme.colBgDeep

    signal openTriggered()
    signal exitTriggered()


    Row {
        anchors.fill: parent
        spacing:      0

        Item {
            id:     fileMenuButton
            width:  70
            height: root.height

            Rectangle {
                anchors.fill:      parent
                bottomRightRadius: 6
                color:             fileMenuMouseArea.containsMouse ? MPTheme.colMenuBgHover : MPTheme.colMenuBg
            }

            Text {
                anchors.centerIn: parent
                text:             "File"
                color:            MPTheme.colTextPrimary
            }

            MouseArea {
                id:           fileMenuMouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked:    fileMenu.open()
            }

            Menu {
                id: fileMenu
                x:  fileMenuButton.x
                y:  root.height

                MenuItem {
                    text: "Open Audio"
                    height: 20
                    onTriggered: root.openTriggered()
                }

                MenuItem {
                    text: "Exit"
                    height: 20
                    onTriggered: root.exitTriggered()
                }
            }
        }
    }
}
