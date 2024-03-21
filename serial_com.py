import sys
import serial
import select
import threading

"""
패킷을 보낼 때 바이트 단위로 전송을 하는 것인가?
아니면 패킷을 보낼 때 바이트 배열로 한번에 묶어서 쏴줘야 하는가?
"""

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

def write(ser, data):
    ser.write(data.encode())

def read(ser, size=1, timeout=None):
    ser.timeout = timeout
    read_data = ser.read(size)
    return read_data

def Identify_Request(ser, header=0xFA, id=0x00, length=0x00, checksum=(0x00, 0x00)):
    packet=bytearray()
    packet.append(header)
    packet.append(id)
    packet.append(length)
    packet.append(checksum[0]); packet.append(checksum[1])
    ser.write(packet)

    #Identify Response

def Device_Info_Request(ser, header=0xFA, id=0x01, length=0x00, checksum=(0x00, 0x00)):
    packet=bytearray()

    print(packet)


def serial_read_thread():
    while True:
        readed = read(ser)
        print("Serial data : ", readed)



ser = openSerial('COM3')

# Serial Thread activate
serial_thread = threading.Thread(target=serial_read_thread, daemon=True)
serial_thread.start()

while True:
    input_data = input("input : ")
    if(input_data =='quit'):
        break
    else:
        Identify_Request(ser)
