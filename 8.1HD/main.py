import sys
import os
from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QGridLayout, QPushButton) 
from PyQt6.QtCore import Qt
from layout import Notification
from widgets import (BleStatusWidget, DimmerSlider, FanButton, PushButton, VoiceCommandWidget)
import bluetooth, MicController, asyncio, qasync

def load_stylesheet(app, filename):
    with open(filename, "r") as f:
        app.setStyleSheet(f.read())

basedir = os.path.dirname(__file__)

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Let There Be Lights 3.0")
        self.setGeometry(0, 0, 700, 500)
        self.ble_connected = False
        self.initUI()
        self.voice_widget = VoiceCommandWidget(self)
        self.ble = BleStatusWidget(self)
        self.ble.button.clicked.connect(self.reconnect)
        asyncio.ensure_future(self.setup())

    def initUI(self):
        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        vbox = QVBoxLayout()
        vbox.setContentsMargins(10, 10, 10, 10)
        top_bar = QHBoxLayout()
        top_bar.addStretch()

        grid = QGridLayout()
        grid.setSpacing(20)
        grid.setAlignment(Qt.AlignmentFlag.AlignCenter)

        exit_button = QPushButton("\u00D7")
        exit_button.setObjectName("exitButton")
        exit_button.clicked.connect(self.closeProgram)
        top_bar.addWidget(exit_button)

        rooms = ["Living Room", "Bathroom", "Closet"]
        fan_button = FanButton("Fan")
        self.buttons = {}

        for i, room in enumerate(rooms):
            button = PushButton(room)
            button.powerLevel = 0
            grid.addWidget(button, 0, i)
            self.buttons[room] = button

        grid.addWidget(fan_button, 1, rooms.index("Bathroom"), Qt.AlignmentFlag.AlignHCenter)
        self.buttons["Fan"] = fan_button

        slider = DimmerSlider("Living Room", on_change=lambda room, val: asyncio.ensure_future(self.on_slider_changed(room, val)))
        self.buttons["Living Room"].slider = slider
        grid.addWidget(slider, 1, rooms.index("Living Room"), Qt.AlignmentFlag.AlignHCenter)

        vbox.addLayout(top_bar)
        vbox.addStretch()
        vbox.addLayout(grid)
        vbox.addStretch()
        central_widget.setLayout(vbox)

    async def setup(self):
        bluetooth.on_lost_connection = self.on_ble_lost
        self.ble_connected = await bluetooth.connect()
        self.update_ble_notification()
        asyncio.ensure_future(MicController.run_mic(self.voice_command))

    @qasync.asyncSlot()
    async def closeProgram(self):
        Notification(self, f"Disconnecting from {bluetooth.DEVICE_NAME}", "blePopup")
        await bluetooth.disconnect()
        self.close()
        sys.exit(0)

    def update_ble_notification(self):
        if self.ble_connected:
            self.ble.set_status("connected")
            self.ble.show_reconnect(False)
            Notification(self, f"Connected to {bluetooth.DEVICE_NAME}", "blePopup")
        else:
            self.ble.set_status("disconnected")
            self.ble.show_reconnect(True)
            self.ble.button.setEnabled(True)
            Notification(self, "Could not connect", "bleError", timeout=0)

    def on_ble_lost(self):
        self.ble_connected = False
        self.ble.set_status("lost")
        self.ble.show_reconnect(True)
        Notification(self, "Bluetooth connection lost", "bleError")

    @qasync.asyncSlot()
    async def reconnect(self):
        self.ble.button.setEnabled(False)
        Notification(self, f"Reconnecting to {bluetooth.DEVICE_NAME}...", "blePopup")
        self.ble_connected = await bluetooth.connect()
        self.update_ble_notification()
        

    async def on_slider_changed(self, room, val):
        button = self.buttons[room]
        button.powerLevel = val
        button.set_active(val > 0)
        await bluetooth.send(room, val)

    async def voice_command(self, power, room):
        caseInsMatch = next((k for k in self.buttons if k.lower() == room.lower()), None)
        if not caseInsMatch:
            return
        button = self.buttons[caseInsMatch]

        if power in (0, 255):
            button.powerLevel = power
            UI_label = "On" if power == 255 else "Off"
        elif power.startswith(('+', '-')):
            adjust = int(power)
            button.powerLevel = max(0, min(255, button.powerLevel + adjust))
            UI_label = "Brighter" if adjust > 0 else "Dimmer"

        button.set_active(button.powerLevel > 0)

        if button.slider is not None:
            button.slider.setValue(button.powerLevel)

        await bluetooth.send(caseInsMatch, button.powerLevel)
        self.voice_widget.show_command(f"Voice: {UI_label} {room}")


def main(): 
    app = QApplication(sys.argv)
    style_path = os.path.join(basedir, "style.qss")
    load_stylesheet(app, style_path)

    loop = qasync.QEventLoop(app)
    asyncio.set_event_loop(loop)

    window = MainWindow()
    window.show()

    with loop: 
        loop.run_forever()

if __name__ == "__main__":
    main()
