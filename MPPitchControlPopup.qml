import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: pitchPopup
    width: 250
    height: 150
    modal: true
    focus: true
    closePolicy: Popup.CloseOnPressOutside

    padding: 16

    background: Rectangle {
        color: "#1B263B"
        border.color: "#415A77"
        border.width: 1
        radius: 8
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Label {
            id: pitchValueLabel
            text: "Pitch Effect Rate: " + parseFloat(pitchSlider.value).toFixed(2) + "x"
            color: "#E0E1DD"
            Layout.alignment: Qt.AlignHCenter
        }

        Slider {
            id: pitchSlider
            Layout.fillWidth: true
            from: 0.5
            to: 2.0
            value: 1.0
            stepSize: 0.01

            onValueChanged: {
                mediaPlayer.setPitch(value)
            }
        }
    }
}
