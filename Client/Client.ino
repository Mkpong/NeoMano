/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 */

#include "BLEDevice.h"

static BLEUUID    serviceUUID("CA5025B1-755F-45FC-AC15-F5E7A7E43008");
static BLEUUID    writeUUID("CA5025B2-755F-45FC-AC15-F5E7A7E43008");
static BLEUUID    notificationUUID("CA5025B3-755F-45FC-AC15-F5E7A7E43008");

byte Identify_Request[5] = {0xFA, 0x00, 0x00, 0x00, 0x00};
byte Device_Info_Request[5] = {0xFA, 0x01, 0x00, 0x00, 0x00};
byte Packet_Table[2][7][7] = {
  // Grip [0][speed]
  {
    {0xFA, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00},
    {0xFA, 0x03, 0x02, 0x00, 0x01, 0x89, 0x11},
    {0xFA, 0x03, 0x02, 0x00, 0x02, 0x12, 0x23},
    {0xFA, 0x03, 0x02, 0x00, 0x03, 0x9B, 0x32},
    {0xFA, 0x03, 0x02, 0x00, 0x04, 0x24, 0x46},
    {0xFA, 0x03, 0x02, 0x00, 0x05, 0xAD, 0x57},
    {0xFA, 0x03, 0x02, 0x00, 0x06, 0x36, 0x65},
  },
  // Release
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

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteWrite;
static BLERemoteCharacteristic* pRemoteNotification;
static BLEAdvertisedDevice* myDevice;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteNotification,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    if(!checksum(pData,length)) return ;
    if(pData[1] == 0xD0) Identify_Response(pData);
    else if(pData[1] == 0xD1) Device_Info_Response(pData);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    pClient->connect(myDevice);  
    Serial.println(" - Connected to server");
    pClient->setMTU(517); 

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our Service");



    pRemoteWrite = pRemoteService->getCharacteristic(writeUUID);
    if (pRemoteWrite == nullptr) {
      Serial.print("Failed to find our Write UUID: ");
      Serial.println(writeUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our Write");


    pRemoteNotification = pRemoteService->getCharacteristic(notificationUUID);
    if (pRemoteNotification == nullptr) {
      Serial.print("Failed to find our Notification UUID: ");
      Serial.println(notificationUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our Notification");

    if(pRemoteWrite->canNotify())
      pRemoteWrite->registerForNotify(notifyCallback);

    if(pRemoteNotification->canNotify())
      pRemoteNotification->registerForNotify(notifyCallback);

    pRemoteWrite->writeValue(Identify_Request, 5);
    delay(1000);
    for(int i = 0; i<5; i++){
      pRemoteWrite->writeValue(&Device_Info_Request[i], 1);
    }
    delay(1000);
    connected = true;
    return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // Serial.print("BLE Advertised Device found: ");
    // Serial.println(advertisedDevice.toString().c_str());

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    }
    else doScan = true;
  }
};

bool checksum(uint8_t *pdata , size_t length){
  return true;
}

// HeartBeat
void Heartbeat_Func(){
  for(int i = 0; i<5; i++){
    pRemoteWrite->writeValue(&Heartbeat[i], 1);
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

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}



String input_string = "";
char *box;
char *command;
int speed, delay_time;

// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    
  }else if(doScan){
    BLEDevice::getScan()->start(5, false);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }
  if(Serial.available()){
    // Input String
    input_string = Serial.readStringUntil('\n');
    box = (char *)input_string.c_str();
    char *token = NULL;
    token = strtok(box, " \n"); command = token;
    token = strtok(NULL, " \n");
    if(token == NULL) speed = -1;
    else { 
      speed = atoi(token);
    }

    switch(speed){
      case 0:
        delay_time = 1800;
        break;
      case 1:
        delay_time = 1800;
        break;
      case 2:
        delay_time = 1800;
        break;
      case 3:
        delay_time = 1800;
        break;
      case 4:
        delay_time = 1800;
        break;
      case 5:
        delay_time = 2000;
        break;
      case 6:
        delay_time = 1800;
        break;
      default:
        delay_time = 0;
        break;
    }


    if(strcmp(command, "grip") == 0 && speed != -1){
      pRemoteWrite->writeValue(Packet_Table[1][speed], 7);
      Serial.print("griping  Speed "); Serial.print(speed); Serial.print("...");
      delay(delay_time);
      pRemoteWrite->writeValue(Sensor_Stop, 5);
      Serial.println("  Complete!");
    }
    else if(strcmp(command, "release") == 0 && speed != -1){
      pRemoteWrite->writeValue(Packet_Table[0][speed], 7);
      Serial.print("releasing  Speed "); Serial.print(speed); Serial.print("...");
      delay(delay_time);
      pRemoteWrite->writeValue(Sensor_Stop, 5);
      Serial.println("  Complete!");
    }
    else{
      Serial.print(command);
      Serial.println(" is Invaild Command");
    }
  }


  delay(1000); // Delay a second between loops.
} // End of loop
