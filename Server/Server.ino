/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updates by chegewara
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID        "CA5025B1-755F-45FC-AC15-F5E7A7E43008"
#define WRITE_UUID          "CA5025B2-755F-45FC-AC15-F5E7A7E43008"
#define NORIFICATION_UUID   "CA5025B3-755F-45FC-AC15-F5E7A7E43008"

byte stop[] = {0xFA, 0x05, 0x01, 0x01, 0x89, 0x11};
byte release[] = {0xFA, 0x05, 0x01, 0x04, 0x24, 0x46};
byte grip[] = {0xFA, 0x05, 0x01, 0x02, 0x12, 0x23};

bool connected = false;

BLECharacteristic *pCharacteristic2 = NULL;

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
void onWrite(BLECharacteristic *pCharacteristic) {
    String value = pCharacteristic->getValue().c_str();
    if (value.length() > 0) {
        Serial.println("Received data from client: ");
        Serial.println(value);
    }
}
};


class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer){
    pCharacteristic2->notify();
    Serial.println("Device Connected");
    connected = true;
  }
  void onDisconnect(BLEServer* pServer){
    Serial.println("Device disconnected");
    connected = false;
    BLEDevice::startAdvertising();
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  BLEDevice::init("NeoMano_8FD8");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         WRITE_UUID,
                                         BLECharacteristic::PROPERTY_WRITE_NR |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

  pCharacteristic2 = pService->createCharacteristic(
                                        NORIFICATION_UUID,
                                        BLECharacteristic::PROPERTY_NOTIFY
                                      );
  pCharacteristic2->setValue(stop, 6);
  pCharacteristic2->addDescriptor(new BLE2902());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}




void gripf() {
  pCharacteristic2->setValue(grip, 6);
  Serial.println("Start Grip");
  for(int i = 0 ;i < 20; i++){
    pCharacteristic2->notify();
    delay(100);
  }
  Serial.println("End Grip");
  pCharacteristic2->setValue(stop, 6);
}

void releasef() {
  pCharacteristic2->setValue(release, 6);
  Serial.println("Start Release");
  for(int i = 0 ;i < 10; i++){
    pCharacteristic2->notify();
    delay(200);
  }
  Serial.println("End Release");
  pCharacteristic2->setValue(stop, 6);
}

void UserInput() {
  String command;
  command = Serial.readStringUntil('\n');
  if(command == "grip") gripf();
  else if(command == "release") releasef();
  else Serial.println("Invaild Command");
}

void loop() {
  // put your main code here, to run repeatedly:
  if(Serial.available()){
    UserInput();
  }
  pCharacteristic2->notify();
  delay(100);
}