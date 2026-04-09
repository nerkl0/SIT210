import sys
import os
from PyQt6.QtWidgets import QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QGridLayout, QPushButton
from PyQt6.QtGui import QIcon
from PyQt6.QtCore import Qt
from app import toggle_pin, cleanup

def load_stylesheet(app, filename):
    with open(filename, "r") as f:
        app.setStyleSheet(f.read())

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        basedir = os.path.dirname(__file__)
        self.setWindowTitle("Let There Be Lights")
        self.setGeometry(0, 0, 700, 500)
        self.initUI()
        icon_path = os.path.join(basedir, "logo.png")
        self.setWindowIcon(QIcon(icon_path))

    def initUI(self):
        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        vbox = QVBoxLayout()
        vbox.setContentsMargins(10, 10, 10, 10)

        top_bar = QHBoxLayout()
        top_bar.addStretch()

        # exit button layout
        exit_button = PushButton("✕", on_click=self.closeProgram)
        exit_button.setObjectName("exitButton")
        top_bar.addWidget(exit_button)

        grid = QGridLayout()
        grid.setSpacing(20)
        grid.setAlignment(Qt.AlignmentFlag.AlignCenter)
        rooms = ["Living Room", "Bathroom", "Closet"]
        
        # creates room buttons within the center widget
        for i, room in enumerate(rooms):
            button = PushButton(room, on_click=self.on_select, selectable=True)
            grid.addWidget(button, 0, i, alignment=Qt.AlignmentFlag.AlignCenter)

        vbox.addLayout(top_bar)
        vbox.addStretch()
        vbox.addLayout(grid)
        vbox.addStretch()
        central_widget.setLayout(vbox)

    # updates styling and calls the app.py toggle_pin function switching LEDs on off
    def on_select(self):
        button = self.sender()
        is_selected = button.property("selected")

        toggle_pin(button.name, is_selected)
        print(f"{button.name} is {'on' if is_selected else 'off'}")

    def closeProgram(self):
        cleanup()


class PushButton(QPushButton):
# Constructor takes on_click handler, which allows passing a method to excute.
# selectable argument allows configuration of button styling when pressing on/off 
    def __init__(self, name, on_click=None, selectable=False):
        super().__init__(name)
        self.name = name 
        self.setCheckable(selectable)
        self.setCursor(Qt.CursorShape.PointingHandCursor)
        self.setProperty("selected", False)

        if selectable:
            self.clicked.connect(self.press_button)
        if on_click:
            self.clicked.connect(on_click)
    
    # sets instance property in qss file to "selected" changing the styling of the pressed button
    def press_button(self):
        current = self.property("selected")
        self.setProperty("selected", not current)
        self.style().unpolish(self)
        self.style().polish(self)
        self.update()

def main(): 
    app = QApplication(sys.argv)
    basedir = os.path.dirname(__file__)
    style_path = os.path.join(basedir, "style.qss")
    load_stylesheet(app, style_path)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()