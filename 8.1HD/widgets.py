from PyQt6.QtWidgets import (QPushButton, QLabel, QWidget, QWidget, QHBoxLayout, QPushButton, QSlider)
from PyQt6.QtCore import (Qt, QTimer)
import qasync, bluetooth

class BleStatusWidget:
    def __init__(self, parent):
        self.parent = parent
        self.indicator = QLabel(parent)
        self.indicator.setObjectName("bleIndicator")
        self.indicator.setFixedSize(16, 16)
        self.button = QPushButton("\u27F3", parent)
        self.button.setObjectName("reconnectButton")
        self.button.setCursor(Qt.CursorShape.PointingHandCursor)
        self.button.setVisible(False)
        self.button.adjustSize()

        # Fixed position: bottom-right of 700x500 window
        margin = 10
        ix = 700 - self.indicator.width() - margin
        iy = 500 - self.indicator.height() - margin
        self.indicator.move(ix, iy)
        self.button.move(ix - self.button.width() - 4, iy + (self.indicator.height() - self.button.height()) // 2)

        self.set_status("disconnected")

    def set_status(self, status: str):
        self.indicator.setProperty("connected", status)
        self.indicator.style().unpolish(self.indicator)
        self.indicator.style().polish(self.indicator)
        self.indicator.update()

    def show_reconnect(self, visible: bool):
        self.button.setVisible(visible)
        self.button.setEnabled(visible)


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

class PushButton(QPushButton):
    def __init__(self, name, slider=None):
        super().__init__(name)
        self.name = name
        self.slider = slider # assigns slider to button
        self.powerLevel = 0
        self.setCheckable(True)
        self.setCursor(Qt.CursorShape.PointingHandCursor)
        self.clicked.connect(self.on_click)

    def set_active(self, state: bool):
        self.setChecked(state)
        if self.slider:
            self.slider.setEnabled(state)
        self.press_button()

    @qasync.asyncSlot(bool)
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

class FanButton(PushButton):
    def __init__(self, name):
        super().__init__(name)
        self.setObjectName("fanButton")
        self.setFixedSize(70, 70)

    def _on_toggled(self, checked):
        self.setProperty("selected", "true" if checked else "false")
        self.style().unpolish(self)
        self.style().polish(self)

class VoiceCommandWidget:
    def __init__(self, parent):
        self.parent = parent

        self.widget = QWidget(parent)
        self.widget.setObjectName("voiceWidget")
        self.widget.setFixedWidth(320)
        self.widget.setFixedHeight(56)

        layout = QHBoxLayout(self.widget)
        layout.setContentsMargins(12, 0, 12, 0)
        layout.setSpacing(8)

        self.icon = QLabel("\u26A1")
        self.icon.setObjectName("voiceIcon")
        self.icon.setFixedSize(24, 24)
        self.icon.setAlignment(Qt.AlignmentFlag.AlignCenter)

        self.label = QLabel("Listening...")
        self.label.setObjectName("voiceLabel")
        self.label.setAlignment(Qt.AlignmentFlag.AlignVCenter | Qt.AlignmentFlag.AlignLeft)

        layout.addWidget(self.icon)
        layout.addWidget(self.label)

        margin = 20
        x = (700 - self.widget.width()) // 2
        y = 500 - self.widget.height() - margin
        self.widget.move(x, y)

        self.widget.setVisible(False)
        self._timer = QTimer()
        self._timer.setSingleShot(True)
        self._timer.timeout.connect(self.hide)

    def show_command(self, text: str, duration_ms: int = 3000):
        self.label.setText(text)
        self.widget.setVisible(True)
        self._timer.start(duration_ms)

    def show_listening(self):
        self._timer.stop()
        self.label.setText("Listening...")
        self.widget.setVisible(True)

    def hide(self):
        self.widget.setVisible(False)