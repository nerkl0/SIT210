from PyQt6.QtWidgets import QLabel
from PyQt6.QtCore import (Qt, QTimer)


# Sits top right of the screen. Provides a 3 second timeout of the notification
class Notification(QLabel):
    def __init__(self, parent, text, style):
        super().__init__(parent)
        self.setText(text)
        self.setObjectName(style)
        
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.adjustSize()
        self.setFixedWidth(260)
        self.move(parent.width() - self.width() - 12, 50)
        self.show()
        
        QTimer.singleShot(3000, self.deleteLater)
