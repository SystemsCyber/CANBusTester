import tkinter as tk
from tkinter import ttk
from tkinter.messagebox import showinfo
import sys
import glob
import serial
import serial.tools.list_ports
from serial import SerialException

def serial_ports():
    if sys.platform.startswith('win'):
        ports = ["COM%s" % (i + 1) for i in range(256)]
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this excludes your current terminal "/dev/tty"
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Unsupported platform')

    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result

class App(tk.Tk):
  def __init__(self):
    super().__init__()

    # configure the root window
    self.title('CANBusTesting')
    self.geometry('300x200')

    # label
    self.label = ttk.Label(self, text='Click start to begin testing')
    self.label.pack(anchor = 'sw',relx = 0, rely = 0)
    # button
    self.button = ttk.Button(self, text='Start')
    self.button['command'] = self.button_clicked
    self.button.pack()

  def button_clicked(self):
    showinfo(title='Information', message='Hello, Tkinter!')

if __name__ == "__main__":
    app = App()
    app.mainloop()

    
