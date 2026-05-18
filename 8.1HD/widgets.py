from PyQt6.QtWidgets import (QPushButton, QLabel, QWidget, QWidget, QHBoxLayout, QPushButton, QSlider)
from PyQt6.QtCore import (Qt, QTimer)
import qasync, bluetooth

'''
    BleStatusWidget sits in the bottom right of the screen providing a visual aid to the current status
    of the bluetooth
    If the status is set to lost, a clickable reconnect button appears  
'''
class BleStatusWidget:
    def __init__(self):
        self.indicator.setObjectName("bleIndicator")
        self.indicator.setFixedSize(16, 16)

        # Reconnect button properties. Initially hidden
        self.button = QPushButton("\u27F3")
        self.button.setObjectName("reconnectButton")
        self.button.setCursor(Qt.CursorShape.PointingHandCursor)
        self.button.setVisible(False)

        # Fixed position: bottom-right of window
        margin = 10
        ix = 700 - self.indicator.width() - margin
        iy = 500 - self.indicator.height() - margin
        self.indicator.move(ix, iy)
        self.button.move(ix - self.button.width() - 4, iy + (self.indicator.height() - self.button.height()) // 2)

        self.set_status("disconnected")

    # Adjusts colour of status widget. Disconnect = red, Connected = green, Lost = yellow
    # Connected is the Button qss property
    def set_status(self, status):
        self.indicator.setProperty("connected", status)
        self.indicator.style().unpolish(self.indicator)
        self.indicator.style().polish(self.indicator)
        self.indicator.update()

    # Toggles reconnect button
    def show_reconnect(self, visible: bool):
        self.button.setVisible(visible)
        self.button.setEnabled(visible)

'''
    DimmerSlider instance can be invoked alongside the button. Initial value is always set to maximum 
'''
class DimmerSlider(QSlider):
    def __init__(self, room_name, on_change=None):
        super().__init__(Qt.Orientation.Vertical)
        self.room_name = room_name
        self.setRange(0, 255) # set range of dim
        self.setValue(255) # initial value 100%
        self.setEnabled(False) # initially disabled, will become active once LED is on

        # if slider instance is enabled, dimmer function will be called, else is ignored
        if on_change:
            self.valueChanged.connect(lambda val: on_change(self.room_name, val) if self.isEnabled() else None)

'''
    PushButton inherits from QPushButton. Used as the room buttons that can be toggled on/off
    on_click() sets either fully on or fully off (no PWM functionality: that can be handled in DimmerSlider instance)

'''
class PushButton(QPushButton):
    def __init__(self, name, slider=None):
        super().__init__(name)
        self.name = name
        self.slider = slider # assigns slider to button
        self.powerLevel = 0 # assigns initial power level (set at zero on initialisation)
        self.setCheckable(True)
        self.setCursor(Qt.CursorShape.PointingHandCursor)
        self.clicked.connect(self.on_click)

    # separates non-asynchronous status changes from the async on_click() 
    def set_active(self, state: bool):
        self.setChecked(state)
        if self.slider:
            self.slider.setEnabled(state)
        self.press_button()

    @qasync.asyncSlot(bool) # qasync decorator required for QT to process async function
    async def on_click(self, state: bool):
        self.powerLevel = 255 if state else 0
        self.set_active(state)
        await bluetooth.send(self.name, 255 if state else 0)
        
    # Handles style update when toggled on/off
    def press_button(self):
        self.setProperty("selected", self.isChecked())
        self.style().unpolish(self)
        self.style().polish(self)
        self.update()

# Inherits from PushButton, allows the fan be styled differently  
class FanButton(PushButton):
    def __init__(self, name):
        super().__init__(name)
        self.setObjectName("fanButton")
        self.setFixedSize(70, 70)

    def on_toggled(self, checked):
        self.setProperty("selected", "true" if checked else "false")
        self.style().unpolish(self)
        self.style().polish(self)

'''
    Widget appears at the bottom of application when a word is recognised from audio input
'''
class VoiceCommandWidget:
    def __init__(self):
        self.widget.setObjectName("voiceWidget")
        self.widget.setFixedWidth(320)
        self.widget.setFixedHeight(56)

        layout = QHBoxLayout(self.widget)
        layout.setContentsMargins(12, 0, 12, 0)
        layout.setSpacing(8)

        self.icon = QLabel("\u26A1") # Unicode lightening emoji 
        self.icon.setObjectName("voiceIcon")
        self.icon.setFixedSize(24, 24)
        self.icon.setAlignment(Qt.AlignmentFlag.AlignCenter)

        layout.addWidget(self.icon)
        layout.addWidget(self.label)

        # position bottom middle of application
        margin = 20
        x = (700 - self.widget.width()) // 2
        y = 500 - self.widget.height() - margin
        self.widget.move(x, y)

        self.widget.setVisible(False)
        self.timer = QTimer()
        self.timer.setSingleShot(True)
        self.timer.timeout.connect(self.hide)

    # show when a command is successfully registered. Disappears after 3 seconds 
    def show_command(self, text, duration_ms: int = 3000):
        self.label.setText(text)
        self.widget.setVisible(True)
        self.timer.start(duration_ms)

    def hide(self):
        self.widget.setVisible(False)