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
        self.ble_state = BleStatusWidget(self)
        self.ble_state.button.clicked.connect(self.reconnect) # assign function to the blestatus widget
        asyncio.ensure_future(self.setup()) # schedules async Setup() as a co-routine for the event loop as await cannot be called in loop 

    def initUI(self):
        # main window base UI layout
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        vbox = QVBoxLayout()
        vbox.setContentsMargins(10, 10, 10, 10)
        top_bar = QHBoxLayout()
        top_bar.addStretch()
        grid = QGridLayout()
        grid.setSpacing(20)
        grid.setAlignment(Qt.AlignmentFlag.AlignCenter)

        # exit button properties
        exit_button = QPushButton("\u00D7")
        exit_button.setObjectName("exitButton")
        exit_button.clicked.connect(self.closeProgram)
        top_bar.addWidget(exit_button)

        # Setup of room/fan buttons
        rooms = ["Living Room", "Bathroom", "Closet"]
        fan_button = FanButton("Fan")
        self.buttons = {}

        # instatiate buttons with default properties and display
        for i, room in enumerate(rooms):
            button = PushButton(room)
            button.powerLevel = 0
            grid.addWidget(button, 0, i)
            self.buttons[room] = button

        grid.addWidget(fan_button, 1, rooms.index("Bathroom"), Qt.AlignmentFlag.AlignHCenter)
        self.buttons["Fan"] = fan_button

        # dimmer slider set only on Living room. Sets on change to take the room and dimmer value to pass to the on_slider_change async function
        slider = DimmerSlider("Living Room", on_change=lambda room, val: asyncio.ensure_future(self.on_slider_change(room, val)))
        self.buttons["Living Room"].slider = slider
        grid.addWidget(slider, 1, rooms.index("Living Room"), Qt.AlignmentFlag.AlignHCenter)

        vbox.addLayout(top_bar)
        vbox.addStretch()
        vbox.addLayout(grid)
        vbox.addStretch()
        central_widget.setLayout(vbox)

### STARTUP - SHUTDOWN METHODS ###
    # Setup called in UI init. Setup bluetooth to maintain instance when lost
    # connects to bluetooth, calling update_ble_notification() to handle the ble notification
    # Sets task for the microphone run_mic() 
    async def setup(self):
        bluetooth.on_lost_connection = self.on_ble_lost
        self.ble_connected = await bluetooth.connect()
        self.update_ble_notification()
        asyncio.ensure_future(MicController.run_mic(self.voice_command))

    # On closing the program, first sever the connecton between client and application
    # if the connection doesn't stop, arduino needs to be reset before being able to use it again
    @qasync.asyncSlot()
    async def closeProgram(self):
        Notification(self, f"Disconnecting from {bluetooth.DEVICE_NAME}", "blePopup")
        await bluetooth.disconnect()
        self.close()
        sys.exit(0)

#### BLUETOOTH RELATED METHODS ####
    # Handles the updating bluetooth notification for connection/disconnection (not on close)
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

    # when connection with bluetooth is lost, update the bluetooth widget, display the reconnect button 
    def on_ble_lost(self):
        self.ble_connected = False
        self.ble.set_status("lost")
        self.ble.show_reconnect(True)
        Notification(self, "Bluetooth connection lost", "bleError")

    # When reconnect button pressed, attempts reconnection by calling bluetooth.connect()
    @qasync.asyncSlot()
    async def reconnect(self):
        self.ble.button.setEnabled(False)
        Notification(self, f"Reconnecting to {bluetooth.DEVICE_NAME}...", "blePopup")
        self.ble_connected = await bluetooth.connect()
        self.update_ble_notification()
        
    # Handles slider change for dimmer slider, set as the on_change event handler for the dimmer slider. 
    async def on_slider_change(self, room, val):
        button = self.buttons[room]
        button.powerLevel = val
        button.set_active(val > 0)
        await bluetooth.send(room, val)

### MIC METHOD ###
    # Handles processing of registered voice command received from the microphone
    # power is the level (PWM) to adjust the device value. 
    # if power is 0 OR 255, the value is either fully on or fully off. 
    # Anything in between is adjusted based on the powerLevel attribute of the button
    async def voice_command(self, power, room):
        # case insensitive matching for the values within the buttons dict, returns if not found
        caseInsMatch = next((k for k in self.buttons if k.lower() == room.lower()), None)
        if not caseInsMatch:
            return
        button = self.buttons[caseInsMatch]

        # configures the power level and updates the UI label for the voice widget
        if power in (0, 255):
            button.powerLevel = power
            UI_label = "On" if power == 255 else "Off"
        else:
            button.powerLevel = max(0, min(255, button.powerLevel + power))
            UI_label = "Brighter" if power > 0 else "Dimmer"

        button.set_active(button.powerLevel > 0) # boolean, if not zero set_active = T : F

        if button.slider is not None:
            button.slider.setValue(button.powerLevel)

        await bluetooth.send(caseInsMatch, button.powerLevel)
        self.voice_widget.show_command(f"Voice: {UI_label} {room}") # display voice command in app


def main(): 
    app = QApplication(sys.argv)
    style_path = os.path.join(basedir, "style.qss")
    load_stylesheet(app, style_path)

    loop = qasync.QEventLoop(app) # setup qa event loop
    asyncio.set_event_loop(loop)

    window = MainWindow()
    window.show()
    
    with loop: 
        loop.run_forever()

if __name__ == "__main__":
    main()
