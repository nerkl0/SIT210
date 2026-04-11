import sys
import os
from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, 
                             QVBoxLayout, QHBoxLayout, QGridLayout, 
                             QPushButton, QSlider) 
from PyQt6.QtGui import QIcon
from PyQt6.QtCore import Qt
from app import dimmer, cleanup, toggle

def load_stylesheet(app, filename):
    with open(filename, "r") as f:
        app.setStyleSheet(f.read())

basedir = os.path.dirname(__file__)

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Let There Be Lights")
        self.setGeometry(0, 0, 700, 500)
        self.initUI()

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
        
        # exit button layout
        exit_button = QPushButton("✕")
        exit_button.setObjectName("exitButton")
        exit_button.clicked.connect(self.closeProgram)
        top_bar.addWidget(exit_button)
    
        rooms = ["Living Room", "Bathroom", "Closet"]

        # Assigns a button and a slider for each room 
        for i, room in enumerate(rooms):
            slider = DimmerSlider(room, on_change=dimmer)
            button = PushButton(room, slider=slider)
            grid.addWidget(button, 0, i)
            grid.addWidget(slider, 1, i, Qt.AlignmentFlag.AlignHCenter)

        vbox.addLayout(top_bar)
        vbox.addStretch()
        vbox.addLayout(grid)
        vbox.addStretch()
        central_widget.setLayout(vbox)

    def closeProgram(self):
        cleanup()
        self.close()


class PushButton(QPushButton):
    def __init__(self, name, slider=None):
        super().__init__(name)
        self.name = name
        self.slider = slider # assigns slider to button
        self.setCheckable(True)
        self.setCursor(Qt.CursorShape.PointingHandCursor)
        self.clicked.connect(self.on_click)

    # when button active, triggers Led to power on and enables slider
    # else if unchecked, slider is disabled and Led turns off
    def on_click(self, checked):
        toggle(self.name, checked)
        if self.slider:
            self.slider.setEnabled(checked)
        self.press_button()
    
    # Handles style update when toggled on/off
    def press_button(self):
        self.setProperty("selected", self.isChecked())
        self.style().unpolish(self)
        self.style().polish(self)
        self.update()

class DimmerSlider(QSlider):
    def __init__(self, room_name, on_change=None):
        super().__init__(Qt.Orientation.Vertical)
        self.room_name = room_name
        self.setRange(0, 100) # set range of dim
        self.setValue(100) # initial value 100%
        self.setEnabled(False) # initially disabled, will become active once LED is on

        # if slider instance is enabled, dimmer function will be called, else is ignored
        if on_change:
            self.valueChanged.connect(lambda val: on_change(room_name, val) if self.isEnabled() else None)

def main(): 
    app = QApplication(sys.argv)
    style_path = os.path.join(basedir, "style.qss")
    load_stylesheet(app, style_path)

    window = MainWindow()
    window.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()