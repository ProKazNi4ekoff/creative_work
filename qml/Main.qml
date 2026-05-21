import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: win
    width: 1240
    height: 780
    visible: true
    title: qsTr("Система умной двери для питомца")

    color: "#12151c"

    property color panel: "#1c2230"
    property color panelAlt: "#242b3a"
    property color accent: "#6ee7b7"
    property color warn: "#fb7185"
    property color textMain: "#f3f6fb"
    property color textMuted: "#9aa8bc"

    function doorStateColor() {
        switch (appController.doorState) {
        case "Open": return accent
        case "Blocked": return warn
        case "Closing": return "#fbbf24"
        default: return textMuted
        }
    }

    function notificationColor(category) {
        if (category === "entry") return accent
        if (category === "exit") return "#60a5fa"
        if (category === "alert") return warn
        if (category === "door") return "#fbbf24"
        if (category === "sim") return textMuted
        return textMain
    }

    FileDialog {
        id: saveCsvDialog
        title: qsTr("Экспорт истории в CSV")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "csv"
        nameFilters: [ qsTr("CSV (*.csv)") ]
        onAccepted: appController.exportCsv(selectedFile)
    }

    header: Column {
        width: win.width
        spacing: 0

        Item { width: 1; height: 14 }

        ToolBar {
            width: parent.width
            background: Rectangle { color: panel }
            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 12

                Label {
                    text: qsTr("Система умной двери для питомца")
                    font.pixelSize: 18
                    font.bold: true
                    color: textMain
                }
                Label {
                    text: qsTr("Роль: %1").arg(appController.role)
                    color: textMuted
                }
                Button {
                    text: qsTr("Роль")
                    flat: true
                    enabled: appController.role === "Owner"
                    onClicked: appController.cycleDemoRole()
                }
                Button {
                    text: qsTr("CSV")
                    flat: true
                    enabled: appController.canExport
                    onClicked: saveCsvDialog.open()
                }
                Item { Layout.fillWidth: true }
            }
        }

        Item { width: 1; height: 12 }

        TabBar {
            id: tabs
            width: parent.width
            TabButton { text: qsTr("Главная") }
            TabButton { text: qsTr("Симуляция") }
            TabButton { text: qsTr("История") }
        }

        Item { width: 1; height: 8 }
    }

    StackLayout {
        anchors.fill: parent
        anchors.margins: 12
        currentIndex: tabs.currentIndex

        // ——— Главная: дверь + уведомления ———
        RowLayout {
            spacing: 12

            Rectangle {
                Layout.preferredWidth: 380
                Layout.fillHeight: true
                radius: 16
                color: panel

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    Label {
                        text: qsTr("Дверь")
                        font.pixelSize: 20
                        font.bold: true
                        color: textMain
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 88
                        radius: 12
                        color: panelAlt
                        border.color: doorStateColor()
                        border.width: 2

                        ColumnLayout {
                            anchors.centerIn: parent
                            Label {
                                text: appController.doorState
                                font.pixelSize: 28
                                font.bold: true
                                color: doorStateColor()
                                Layout.alignment: Qt.AlignHCenter
                            }
                            Label {
                                text: appController.obstacle
                                    ? qsTr("Препятствие в проёме")
                                    : qsTr("Проём свободен")
                                color: appController.obstacle ? warn : accent
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        Button {
                            Layout.fillWidth: true
                            text: qsTr("Открыть")
                            enabled: appController.canRemoteOpen
                            highlighted: true
                            onClicked: appController.openDoor()
                        }
                        Button {
                            Layout.fillWidth: true
                            text: qsTr("Закрыть")
                            enabled: appController.canRemoteOpen
                            onClicked: appController.closeDoor()
                        }
                    }

                    Label {
                        visible: appController.closeRetryPending
                        text: qsTr("Повторное закрытие через 15 сек…")
                        color: warn
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Label {
                        visible: appController.waitingForOpenDoor
                        text: qsTr("Питомец у двери (чип). Откройте дверь.")
                        color: accent
                        font.bold: true
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: "#334155" }

                    Label {
                        text: qsTr("Вход и выход")
                        font.pixelSize: 16
                        font.bold: true
                        color: textMuted
                    }
                    Label {
                        text: qsTr("Последний вход: %1").arg(appController.lastHomeEntry)
                        color: textMain
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Label {
                        text: qsTr("Последний выход: %1").arg(appController.lastExit)
                        color: textMain
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Label {
                        text: qsTr("На улице: %1").arg(appController.timeOutside)
                        color: textMuted
                    }

                    Label {
                        text: qsTr("Вход и выход фиксируются автоматически по времени открытия двери.")
                        color: textMuted
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 16
                color: panel

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 8

                    Label {
                        text: qsTr("Уведомления")
                        font.pixelSize: 20
                        font.bold: true
                        color: textMain
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 6
                        model: appController.notifications

                        delegate: Rectangle {
                            width: ListView.view.width
                            implicitHeight: msgText.implicitHeight + 28
                            radius: 8
                            color: index % 2 === 0 ? panelAlt : "#1a2030"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 4

                                Label {
                                    text: modelData.ts
                                    color: textMuted
                                    font.pixelSize: 12
                                }
                                Label {
                                    id: msgText
                                    text: modelData.text
                                    color: notificationColor(modelData.category || "info")
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: appController.notifications.length === 0
                            text: qsTr("Пока нет уведомлений")
                            color: textMuted
                        }
                    }
                }
            }
        }

        // ——— Симуляция ———
        RowLayout {
            spacing: 12

            Rectangle {
                Layout.preferredWidth: 380
                Layout.fillHeight: true
                radius: 16
                color: panel

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 10

                    Label {
                        text: qsTr("Симуляция RFID")
                        font.pixelSize: 20
                        font.bold: true
                        color: textMain
                    }
                    Label {
                        text: qsTr("Имитация чипа на ошейнике у двери. После считывания откройте дверь на «Главной» — вход зафиксируется по времени открытия.")
                        color: textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: "#334155" }

                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Считать чип у двери")
                        onClicked: appController.simulateChipAtDoor()
                    }

                    Switch {
                        text: qsTr("Препятствие в проёме")
                        checked: appController.obstacle
                        onToggled: appController.setObstacle(checked)
                    }
                    Label {
                        text: qsTr("При препятствии дверь не закроется; повтор каждые 15 сек.")
                        color: warn
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: "#334155" }

                    Label { text: qsTr("Автосимуляция"); font.bold: true; color: textMuted }
                    Label {
                        text: qsTr("Считывает чип и ждёт, пока вы нажмёте «Открыть» на главной.")
                        color: textMuted
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Button {
                            Layout.fillWidth: true
                            text: qsTr("Запустить")
                            enabled: !appController.simulationRunning
                            onClicked: appController.startAutoSimulation()
                        }
                        Button {
                            Layout.fillWidth: true
                            text: qsTr("Стоп")
                            enabled: appController.simulationRunning
                            onClicked: appController.stopAutoSimulation()
                        }
                    }

                    Label {
                        visible: appController.simulationRunning && appController.waitingForOpenDoor
                        text: qsTr("Ожидание: откройте дверь на «Главной»")
                        color: accent
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Item { Layout.fillHeight: true }

                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Очистить историю симуляции")
                        onClicked: appController.clearSimHistory()
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 16
                color: panel

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 8

                    Label {
                        text: qsTr("История симуляции")
                        font.pixelSize: 20
                        font.bold: true
                        color: textMain
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 4
                        model: appController.simEvents

                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 52
                            radius: 8
                            color: index % 2 === 0 ? panelAlt : "#1a2030"

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 10
                                Label {
                                    text: modelData.ts
                                    color: textMuted
                                    Layout.preferredWidth: 150
                                }
                                Label {
                                    text: modelData.type
                                    color: "#93c5fd"
                                    Layout.preferredWidth: 120
                                }
                                Label {
                                    text: modelData.message
                                    color: textMain
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: appController.simEvents.length === 0
                            text: qsTr("Событий симуляции пока нет")
                            color: textMuted
                        }
                    }
                }
            }
        }

        // ——— История ———
        Rectangle {
            radius: 16
            color: panel

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("История приложения")
                        font.pixelSize: 20
                        font.bold: true
                        color: textMain
                        Layout.fillWidth: true
                    }
                    Button {
                        text: qsTr("Очистить")
                        flat: true
                        onClicked: appController.clearMainHistory()
                    }
                }

                Label {
                    text: qsTr("Только реальные действия: дверь, вход, выход (без симуляции).")
                    color: textMuted
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 4
                    model: appController.events

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 52
                        radius: 8
                        color: index % 2 === 0 ? panelAlt : "#1a2030"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 10
                            Label {
                                text: modelData.ts
                                color: textMuted
                                Layout.preferredWidth: 150
                            }
                            Label {
                                text: modelData.type
                                color: "#93c5fd"
                                Layout.preferredWidth: 140
                            }
                            Label {
                                text: modelData.message
                                color: textMain
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }
        }
    }
}
