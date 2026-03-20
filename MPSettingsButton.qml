import QtQuick
import QtQuick.Controls
import FatPound.MediaPlayer 0.1

ToolButton {
    id: root
    width:  32
    height: 32

    signal clicked_()

    contentItem: Image {
        source:            "qrc:/assets/tune_effects.png"
        sourceSize.width:  24
        sourceSize.height: 24
        fillMode:          Image.PreserveAspectFit
        opacity:           0.7
    }

    onClicked: root.clicked_()
}
