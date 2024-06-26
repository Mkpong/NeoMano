'''
ESP32 Serial Communication Code
Author : Jaeyoung Lee
Mail : leeja042499@gmail.com
Data : 2024.03
'''

import serial
import threading
import argparse

# Argument Parser 설정
parser = argparse.ArgumentParser(description='Serial Communication Code')
parser.add_argument('--port', type=str, default='COM3', help='Serial port name (e.g., COM3)')

args = parser.parse_args()

port = args.port

# Open Serial Port
def openSerial(port, baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=None, xonxoff=False, rtscts=False, dsrdtr=False):
    ser = serial.Serial()

    ser.port = port
    ser.baudrate = baudrate
    ser.bytesize = bytesize
    ser.parity = parity
    ser.stopbits = stopbits
    ser.timeout = timeout
    ser.xonxoff = xonxoff
    ser.rtscts = rtscts
    ser.dsrdtr = False

    ser.open()
    return ser

# read data from Serial port
def read(ser, size=1):
    try:
        read_data = ser.read(size)
    except serial.SerialTimeoutException:
        return -1
    return read_data

# read function - Thread
def serial_read_thread():
    while True:
        readed = read(ser)
        try:
            print(readed.decode('utf-8'), end="")
        except UnicodeDecodeError: # utf-8 decodeing이 가능한 문자열만 출력
            print("", end="")



# Enter Port number you use
ser = openSerial(port)

# Sending serial data - Thread
serial_thread = threading.Thread(target=serial_read_thread, daemon=True)
serial_thread.start()


while True:
    input_data = input("")
    ser.write(input_data.encode('utf-8'))
