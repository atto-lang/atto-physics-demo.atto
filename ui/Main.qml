import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import OresPhysics 1.0

ApplicationWindow {
    id: window
    width: 1180
    height: 760
    minimumWidth: 700
    minimumHeight: 510
    visible: true
    color: "#080f1c"
    title: "Oreslang | Native LLVM Physics"

    Rectangle {
        anchors.fill: parent
        color: "#080f1c"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            spacing: 16
            ColumnLayout {
                Text {
                    text: "ORESLANG  /  PHYSICS"
                    color: "#eef5ff"
                    font.pixelSize: 23
                    font.bold: true
                    font.letterSpacing: 2
                }
                Text {
                    text: "AOT LLVM guest · fixed-point · preallocated SoA · Qt Quick skin"
                    color: "#90a6c4"
                    font.pixelSize: 12
                }
            }
            Item { Layout.fillWidth: true }
            Button {
                text: field.paused ? "Resume" : "Pause"
                onClicked: field.paused = !field.paused
            }
            Button {
                text: "Reset"
                onClicked: field.reset()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#0d1626"
            border.color: "#273d59"
            border.width: 1
            radius: 12
            clip: true
            PhysicsCanvas {
                id: field
                anchors.fill: parent
                Timer {
                    interval: 16
                    repeat: true
                    running: true
                    onTriggered: field.refresh()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 28
            Text { text: "Bodies: 192"; color: "#d7e5fa"; font.pixelSize: 14 }
            Text { text: "Fixed timestep: 60 Hz"; color: "#d7e5fa"; font.pixelSize: 14 }
            Text { text: "Tick: " + field.tick; color: "#d7e5fa"; font.pixelSize: 14 }
            Text { text: "Late frames: " + field.missedDeadlines; color: "#dfbc8e"; font.pixelSize: 14 }
            Text { text: "Skipped visual snapshots: " + field.droppedSnapshots; color: "#90a6c4"; font.pixelSize: 14 }
            Item { Layout.fillWidth: true }
        }
        Text {
            text: "Physics: compiled .ores functions  |  Qt is presentation only  |  Hard real-time certification pending"
            color: "#66809e"
            font.pixelSize: 11
        }
    }
}
