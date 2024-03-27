import sys
import serial
import select
import threading


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

def read(ser, size=1):
    try:
        read_data = ser.read(size)
    except serial.SerialTimeoutException:
        return -1
    return read_data


# Request Function Start

def Identify_Request(ser, header=0xFA, id=0x00, length=0x00, checksum=(0x00, 0x00), args=["identify"]):
    packet=bytearray()
    packet.append(header)
    packet.append(id)
    packet.append(length)
    # packet.append(checksum[0]); packet.append(checksum[1])
    ser.write(packet)

def Device_Info_Request(ser, header=0xFA, id=0x01, length=0x00, checksum=(0x00, 0x00), args=["device_info"]):
    packet=bytearray()
    packet.append(header)
    packet.append(id)
    packet.append(length)
    # packet.append(checksum[0]); packet.append(checksum[1])
    ser.write(packet)

def Write_Request(ser, header=0xFA, id=0x90, length=0x05, checksum=(0x00, 0x00), args=["write"]):
    # 입력 데이터의 길이가 4byte가 아닌 경우
    if len(args)!=6:
        print("Input invaild Data")
        return
    packet=bytearray()
    packet.append(header)
    packet.append(id)
    packet.append(length)
    packet.append(int(args[1], 16))
    for i in range(4):
        packet.append(int(args[i+2], 16))
    # packet.append(checksum[0]); packet.append(checksum[1])
    print(packet)
    ser.write(packet)

def EEPROM_Data_Request(ser, header=0xFA, id=0x91, length=0x01, checksum=(0x00, 0x00), args=["get_eeprom"]):
    if len(args)!=2:
        print("Input invaild Data")
        return
    packet = bytearray()
    packet.append(header)
    packet.append(id)
    packet.append(length)
    packet.append(int(args[1], 16))
    ser.write(packet)

def BLE_Name_Request(ser, header=0xFA, id=0x92, length=0x01, checksum=(0x00, 0x00), args=["ble_name"]):
    length = len(args[1])
    packet = bytearray()
    packet.append(header)
    packet.append(id)
    packet.append(length)
    byte_data = [ord(char) for char in args[1]]
    for data in byte_data:
        packet.append(data)
    ser.write(packet)

# Request Function End


# Response Function Start

def Identify_Response(ser, payload, checksum):
    # Checksum Process

    # Payload Process
    product_ID = payload[0]
    Firmware_Version = []
    for i in range(3):
        Firmware_Version.append(int.from_bytes(payload[i+1], byteorder='big'))
    print(f"Product ID : {int.from_bytes(product_ID, byteorder='big')}, Version : {Firmware_Version[0]}.{Firmware_Version[1]}.{Firmware_Version[2]}")
    

def Device_Info_Response(ser, payload, checksum):
    # Payload Process
    if(payload[0] == 0x00):
        print(f"device battery is insufficient")
    else:
        print(f"Device battery is sufficient")

def Power_Off(ser, payload, checksum):
    print("Device Disconnected")

def Write_Response(ser, payload, checksum):
    # Payload Process
    address = payload[0]
    byte_data = payload[1:5]
    data = [hex(int.from_bytes(byte, byteorder='big')) for byte in byte_data]
    print(f"주소({address})에 Data{data}가 저장되었습니다.")

def EEPROM_Data_Response(ser, payload, checksum):
    #Payload Process
    address = payload[0]
    byte_data = payload[1:5]
    data = [hex(int.from_bytes(byte, byteorder='big')) for byte in byte_data]
    print(f"주소({address})에 저장된 데이터 : {data}")

def BLE_Name_Response(ser, payload, checksum):
    print("A")
    # payload에 length추가해야 할듯 length를 사용하는 데이터가 존재

def Sensor_Data(ser, payload, checksum):
    print("Sensor Control status")


# Response Function End

def Input_Packet(ser):
    payload = []
    checksum = []
    ID_data = read(ser)
    # print(ID_data)
    # Timeout Exception
    if(ID_data == -1):
        return
    
    Length_data = read(ser)
    # Timeout Exception
    if(Length_data == -1):
        return
    
    for i in range(int.from_bytes(Length_data, byteorder='big')):
        payload.append(read(ser))
    
    # for i in range(2):
    #     checksum.append(read(ser))
    Response_Function.get(ID_data, Packet_Error)(ser, payload, checksum)

def serial_read_thread():
    while True:
        readed = read(ser)
        if readed == b'\xfa':
            Input_Packet(ser)

def Input_Error(ser, args):
    print(f"{command[0]} is Invalid command")

def Packet_Error(ser, payload, checksum):
    print("Invalid Packet")



ser = openSerial('COM3')
Response_Function = {
    b'\xd0': Identify_Response,
    b'\xd1': Device_Info_Response,
    b'\xd2': Power_Off,
    b'\xE0': Write_Response,
    b'\xE1': EEPROM_Data_Response,
    b'\xE2': BLE_Name_Response,
    b'\xD3': Sensor_Data,
}
Request_Function = {
    "identify": Identify_Request,
    "device_info": Device_Info_Request,
    "write": Write_Request,
    "get_eeprom": EEPROM_Data_Request,
    "ble_name": BLE_Name_Request,
}

# Serial Thread activate
serial_thread = threading.Thread(target=serial_read_thread, daemon=True)
serial_thread.start()

while True:
    input_data = input("")
    if(input_data =='quit'):
        break
    elif(input_data ==""):
        print("Not Input Command")
    else:
        command = input_data.split()
        Request_Function.get(command[0],Input_Error)(ser, args=command)
