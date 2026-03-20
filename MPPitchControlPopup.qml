import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FatPound.MediaPlayer 0.1

Popup {
    id: root
    width:       250
    height:      130
    modal:       true
    focus:       true
    padding:     16
    closePolicy: Popup.CloseOnPressOutside

    signal pitchChanged(real value)

    background:
        Rectangle {
            color:        MPTheme.colBgMid
            border.color: MPTheme.colBorder
            border.width: 1
            radius:       8
        }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Label {
            text:             "Pitch: " + parseFloat(pitchSlider.value).toFixed(2) + "x"
            color:            MPTheme.colTextPrimary
            Layout.alignment: Qt.AlignHCenter
        }

        Slider {
            id:               pitchSlider
            Layout.fillWidth: true
            from:             0.5
            to:               2.0
            value:            1.0
            stepSize:         0.01
            onValueChanged:   root.pitchChanged(value)
        }
    }
}
