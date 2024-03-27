/* 
* NeoMano control program used esp32
* Author : Jaeyoung Lee
* Email : leeja042499@gmail.com
* Date : 2024.03
*/

#include "BLEDevice.h"

static BLEUUID    serviceUUID("CA5025B1-755F-45FC-AC15-F5E7A7E43008");
static BLEUUID    writeUUID("CA5025B2-755F-45FC-AC15-F5E7A7E43008");
static BLEUUID    notificationUUID("CA5025B3-755F-45FC-AC15-F5E7A7E43008");

byte Identify_Request[5] = {0xFA, 0x00, 0x00, 0x00, 0x00};
byte Device_Info_Request[5] = {0xFA, 0x01, 0x00, 0x00, 0x00};
byte Packet_Table[2][7][7] = {
  // release
  {
    {0xFA, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00},
    {0xFA, 0x03, 0x02, 0x00, 0x01, 0x89, 0x11},
    {0xFA, 0x03, 0x02, 0x00, 0x02, 0x12, 0x23},
    {0xFA, 0x03, 0x02, 0x00, 0x03, 0x9B, 0x32},
    {0xFA, 0x03, 0x02, 0x00, 0x04, 0x24, 0x46},
    {0xFA, 0x03, 0x02, 0x00, 0x05, 0xAD, 0x57},
    {0xFA, 0x03, 0x02, 0x00, 0x06, 0x36, 0x65},
  },
  // grip
  {
    {0xFA, 0x03, 0x02, 0x01, 0x00, 0xD8, 0x19},
    {0xFA, 0x03, 0x02, 0x01, 0x01, 0x51, 0x08},
    {0xFA, 0x03, 0x02, 0x01, 0x02, 0xCA, 0x3A},
    {0xFA, 0x03, 0x02, 0x01, 0x03, 0x43, 0x2B},
    {0xFA, 0x03, 0x02, 0x01, 0x04, 0xFC, 0x5F},
    {0xFA, 0x03, 0x02, 0x01, 0x05, 0x75, 0x4E},
    {0xFA, 0x03, 0x02, 0x01, 0x06, 0xEE, 0x7C},
  },
};
byte Sensor_Stop[5] = {0xFA, 0x04, 0x00, 0x00, 0x00};
byte Heartbeat[5] = {0xFA, 0x9F, 0x00, 0x00, 0x00};

const uint16_t crc_table[256] = {
  0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
  0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
  0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
  0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
  0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
  0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
  0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
  0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
  0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
  0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
  0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
  0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
  0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
  0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
  0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
  0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
  0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
  0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
  0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
  0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
  0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
  0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
  0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
  0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
  0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3, 
  0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
  0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
  0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
  0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
  0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
  0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
  0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteWrite;
static BLERemoteCharacteristic* pRemoteNotification;
static BLEAdvertisedDevice* myDevice;

String input_string = "";
char *box;
char *command;
int speed, delay_time;

// Callback Function (Receving Packet)
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteNotification,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    // Device -> Host Packet Processing
    if(!checkSum_Verification(pData,length)) return ; // Checksum Verification Process
    // Check ID & Function Call
    if(pData[1] == 0xD0) Identify_Response(pData);
    else if(pData[1] == 0xD1) Device_Info_Response(pData);
}

// Callback function (Bluetooth connection(client) event)
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("OnConnect")
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

// Connect to device & Find service and add characteristic(write, notification)
bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    pClient->connect(myDevice);  
    Serial.println(" - Connected to server");
    pClient->setMTU(517); 

    // Find Service
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our Service");


    // Find write characteristic
    pRemoteWrite = pRemoteService->getCharacteristic(writeUUID);
    if (pRemoteWrite == nullptr) {
      Serial.print("Failed to find our Write UUID: ");
      Serial.println(writeUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our Write");

    // Find notification chracteristic
    pRemoteNotification = pRemoteService->getCharacteristic(notificationUUID);
    if (pRemoteNotification == nullptr) {
      Serial.print("Failed to find our Notification UUID: ");
      Serial.println(notificationUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our Notification");

    // Callback function setting
    if(pRemoteWrite->canNotify())
      pRemoteWrite->registerForNotify(notifyCallback);

    if(pRemoteNotification->canNotify())
      pRemoteNotification->registerForNotify(notifyCallback);

    // Connection HandShake & Check Battery
    pRemoteWrite->writeValue(Identify_Request, 5);
    delay(1000);
    pRemoteWrite->writeValue(&Device_Info_Request[i], 1);
    delay(1000);

    connected = true;
    return true;
}

// Callback function when find serviceUUID
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    }
    else doScan = true;
  }
};


// HeartBeat
void Heartbeat_Func(){
  for(int i = 0; i<5; i++){
    pRemoteWrite->writeValue(Heartbeat, 5);
  }
}

//Identify Function (Product ID & Firmware Version)
void Identify_Response(uint8_t* packet){
  Serial.print("Device ID : ");
  Serial.print(packet[3]);
  Serial.print(" / Version : ");
  Serial.print(packet[4]);
  Serial.print(".");
  Serial.print(packet[5]);
  Serial.print(".");
  Serial.println(packet[6]);
}

// Device_Info Funcetion (Battery Condition)
void Device_Info_Response(uint8_t* packet){
  if(packet[3] == 1) Serial.println("Battery High");
  else if(packet[3] == 0) Serial.println("Battery Low");
}

// Verifty Checksum (for Receving Packet)
bool checkSum_Verification(uint8_t *pData, size_t length){
  uint8_t checksum[2];
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false); // scan to run for 5 sec
}

void UserInput(){
  char *charData, *args[10], *command, *token = NULL;
  int argc = 0;
  String inputData = Serial.readStringUntil('\n');
  charData = (char *)inputData.c_str();
  token = strtok(charData, " \n"); command = token;
  while(token != NULL){
    if(argc > 10) break;
    token = strtok(NULL, " \n");
    strcpy(args[argc] , token);
    argc++;
  }
  checkCommand(command, args);
}

void CheckCommand(char *command, char *args[10]){
  
}

// This is the Arduino main loop function.
void loop() {


  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  if (connected) {
    
  }else if(doScan){
    // scan to run for 5 sec
    BLEDevice::getScan()->start(5, false);
  }


  if(Serial.available()){
    // Input Command Process
    input_string = Serial.readStringUntil('\n');
    box = (char *)input_string.c_str();
    char *token = NULL;
    token = strtok(box, " \n"); command = token;
    token = strtok(NULL, " \n");
    if(token == NULL) speed = -1;
    else { 
      speed = atoi(token);
    }
    token = strtok(NULL, " \n");
    if(token == NULL) delay_time = -1;
    else {
      delay_time = atoi(token);
    }
    
    // Check Command & Sending Request Packet
    if(strcmp(command, "grip") == 0 && speed != -1 && delay_time != -1){
      pRemoteWrite->writeValue(Packet_Table[1][speed], 7);
      Serial.print("griping  Speed "); Serial.print(speed); Serial.print("...");
      delay(delay_time);
      pRemoteWrite->writeValue(Sensor_Stop, 5);
    }
    else if(strcmp(command, "release") == 0 && speed != -1 && delay_time != -1){
      pRemoteWrite->writeValue(Packet_Table[0][speed], 7);
      Serial.print("releasing  Speed "); Serial.print(speed); Serial.print("...");
      delay(delay_time);
      pRemoteWrite->writeValue(Sensor_Stop, 5);
    }
    else if(strcmp(command, "stop") == 0){
      pRemoteWrite->writeValue(Sensor_Stop, 5);
    }
    else{
      Serial.print(command);
      Serial.println(" is Invaild Command");
    }
  }


  delay(1000); // Delay a second between loops.
} // End of loop
