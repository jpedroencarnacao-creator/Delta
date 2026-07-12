import sys
from PySide6.QtWidgets import (
    QApplication, QMainWindow, QWidget,
    QLabel, QPushButton, QVBoxLayout, QHBoxLayout,
    QPlainTextEdit, QFrame, QLineEdit, QDialog
)
from PySide6.QtCore import Qt


def print_pi(*args):
    print("PI_4B: ", *args)


class PasswordDialog(QDialog):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("Acesso DEV")
        self.setFixedSize(260, 120)

        layout = QVBoxLayout()

        self.label = QLabel("Introduza a password DEV:")
        self.password_input = QLineEdit()
        self.password_input.setEchoMode(QLineEdit.Password)

        self.ok_button = QPushButton("Entrar")
        self.ok_button.clicked.connect(self.accept)

        layout.addWidget(self.label)
        layout.addWidget(self.password_input)
        layout.addWidget(self.ok_button)

        self.setLayout(layout)

    def get_password(self):
        return self.password_input.text()


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("PI_4B Control Panel")
        self.setMinimumSize(1000, 600)

        self.dev_mode = False
        self.last_serial_command = ""

        main_widget = QWidget()
        self.setCentralWidget(main_widget)

        self.main_layout = QHBoxLayout()
        self.main_layout.setContentsMargins(12, 12, 12, 12)
        self.main_layout.setSpacing(12)
        main_widget.setLayout(self.main_layout)

        self.left_panel = QWidget()
        self.left_layout = QVBoxLayout()
        self.left_layout.setSpacing(12)
        self.left_panel.setLayout(self.left_layout)

        self.status_label = QLabel("Modo atual: CLIENTE")
        self.status_label.setAlignment(Qt.AlignCenter)

        self.mode_button = QPushButton("Modo CLIENTE")
        self.mode_button.setFixedSize(120, 40)
        self.mode_button.clicked.connect(self.toggle_mode)

        self.close_button = QPushButton("Sair")
        self.close_button.setFixedSize(120, 40)
        self.close_button.clicked.connect(self.close)

        self.left_layout.addWidget(self.status_label)
        self.left_layout.addWidget(self.mode_button, alignment=Qt.AlignLeft | Qt.AlignTop)
        self.left_layout.addStretch()
        self.left_layout.addWidget(self.close_button, alignment=Qt.AlignLeft | Qt.AlignBottom)

        self.serial_panel = QFrame()
        self.serial_panel.setFrameShape(QFrame.StyledPanel)
        self.serial_panel.setMinimumWidth(340)

        self.serial_layout = QVBoxLayout()
        self.serial_layout.setContentsMargins(10, 10, 10, 10)
        self.serial_layout.setSpacing(10)
        self.serial_panel.setLayout(self.serial_layout)

        self.serial_title = QLabel("Monitor Serial")
        self.serial_title.setAlignment(Qt.AlignCenter)

        self.serial_monitor = QPlainTextEdit()
        self.serial_monitor.setReadOnly(True)
        self.serial_monitor.setPlainText(
            "Zona reservada para mensagens enviadas e recebidas pela serial.\n"
            "A comunicação será ligada mais tarde."
        )

        self.serial_input = QLineEdit()
        self.serial_input.setPlaceholderText("Escrever comando para enviar pela serial...")

        self.send_button = QPushButton("Enviar")
        self.send_button.clicked.connect(self.send_serial_command)

        self.serial_input_layout = QHBoxLayout()
        self.serial_input_layout.setSpacing(8)
        self.serial_input_layout.addWidget(self.serial_input)
        self.serial_input_layout.addWidget(self.send_button)

        self.serial_layout.addWidget(self.serial_title)
        self.serial_layout.addWidget(self.serial_monitor)
        self.serial_layout.addLayout(self.serial_input_layout)

        self.serial_panel.hide()

        self.main_layout.addWidget(self.left_panel, 3)
        self.main_layout.addWidget(self.serial_panel, 2)

        self.setStyleSheet("""
            QMainWindow {
                background-color: white;
            }

            QWidget {
                background-color: white;
                color: black;
                font-size: 14px;
            }

            QLabel {
                font-size: 16px;
                font-weight: bold;
            }

            QPushButton {
                background-color: #e8e8e8;
                border: 1px solid #bdbdbd;
                border-radius: 6px;
                padding: 10px;
                min-height: 18px;
            }

            QPushButton:hover {
                background-color: #dcdcdc;
            }

            QFrame {
                background-color: #f7f7f7;
                border: 1px solid #cfcfcf;
                border-radius: 8px;
            }

            QPlainTextEdit {
                background-color: white;
                border: 1px solid #cfcfcf;
                border-radius: 6px;
                padding: 6px;
            }

            QLineEdit {
                background-color: white;
                border: 1px solid #a8a8a8;
                border-radius: 6px;
                padding: 8px;
                min-height: 18px;
            }
        """)

        self.update_mode_button_style()
        self.update_close_button_style()

    def update_mode_button_style(self):
      if self.dev_mode:
        self.mode_button.setText("Modo DEV")
        self.mode_button.setStyleSheet("""
            QPushButton {
                background-color: #ffd84d;
                color: black;
                border: 2px solid #d9534f;
                border-radius: 14px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #ffcf33;
            }
        """)
      else:
        self.mode_button.setText("Modo CLIENTE")
        self.mode_button.setStyleSheet("""
            QPushButton {
                background-color: #337ab7;
                color: white;
                border: 2px solid #8ec5ff;
                border-radius: 14px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #286090;
            }
        """)

    def update_close_button_style(self):
        self.close_button.setStyleSheet("""
            QPushButton {
                background-color: #d9534f;
                color: white;
                border: 1px solid #b52b27;
                border-radius: 6px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #c9302c;
            }
        """)

    def toggle_mode(self):
        if not self.dev_mode:
            dialog = PasswordDialog()
            if dialog.exec() == QDialog.Accepted:
                if dialog.get_password() == "1234":
                    self.dev_mode = True
                    self.status_label.setText("Modo atual: DEV")
                    self.serial_panel.show()
                    print("Modo DEV ativado")
                else:
                    print("Password DEV incorreta")
                    return
            else:
                return
        else:
            self.dev_mode = False
            self.status_label.setText("Modo atual: CLIENTE")
            self.serial_panel.hide()
            print("Modo CLIENTE ativado")

        self.update_mode_button_style()

    def send_serial_command(self):
        command = self.serial_input.text().strip()

        if not command:
            return

        self.last_serial_command = command
        self.serial_input.clear()

        print_pi(command)
        self.serial_monitor.appendPlainText(f"PI_4B:  {command}")


app = QApplication(sys.argv)
window = MainWindow()
window.showMaximized()
app.exec()