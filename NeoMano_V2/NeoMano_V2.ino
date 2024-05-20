/* 
* NeoMano control program used esp32 Version. 2
* Author : Jaeyoung Lee
* Email : leeja042499@gmail.com
* Date : 2024.05
*/

#include "BLEDevice.h"


static BLEUUID    serviceUUID("CA5025B1-755F-45FC-AC15-F5E7A7E43008");
static BLEUUID    writeUUID("CA5025B2-755F-45FC-AC15-F5E7A7E43008");
static BLEUUID    notificationUUID("CA5025B3-755F-45FC-AC15-F5E7A7E43008");

byte Identify_Request[5] = {0xFA, 0x00, 0x00, 0x00, 0x00};
byte Device_Info_Request[5] = {0xFA, 0x01, 0x00, 0x00, 0x00};
byte Sensor_Stop[5] = {0xFA, 0x04, 0x00, 0x00, 0x00};
// grip 및 release 요청을 위한 Packet Table
// [Release(0), Grip(1)][Speed][PacketData]
byte Packet_Table[2][7][7] = {
  {
    {0xFA, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00},
    {0xFA, 0x03, 0x02, 0x00, 0x01, 0x89, 0x11},
    {0xFA, 0x03, 0x02, 0x00, 0x02, 0x12, 0x23},
    {0xFA, 0x03, 0x02, 0x00, 0x03, 0x9B, 0x32},
    {0xFA, 0x03, 0x02, 0x00, 0x04, 0x24, 0x46},
    {0xFA, 0x03, 0x02, 0x00, 0x05, 0xAD, 0x57},
    {0xFA, 0x03, 0x02, 0x00, 0x06, 0x36, 0x65},
  },
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

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteWrite;
static BLERemoteCharacteristic* pRemoteNotification;
static BLEAdvertisedDevice* myDevice;
uint8_t counter = 0;
hw_timer_t *timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
static boolean stopflag = false;
static boolean workingFlag = false;
static boolean lastTaskFlag = false;

int delay_time[7] = {1000, 300000, 10000, 6000, 4000, 3000, 2500};


struct dataPacket {
  boolean direction;
  int speed;
  int delayTime;
};

struct QueueNode {
  struct dataPacket data;
  struct QueueNode* next;
};

struct QueueNode *header = NULL;
struct QueueNode *tail = NULL;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteNotification,
  uint8_t* pData,
  size_t length,
  bool isNotify);

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
public:
    void onResult(BLEAdvertisedDevice advertisedDevice);
};

class MyClientCallback: public BLEClientCallbacks {
public:
    void onConnect(BLEClient* pclient);
    void onDisconnect(BLEClient* pclient);
};

ARDUINO_ISR_ATTR void IRAM_ATTR timerCallback();

void grip(char *args[10], int argc);
void release(char *args[10], int argc);
void griprelease(char *args[10], int argc);
void releasegrip(char *args[10], int argc);
void stop();
void identify();
void response();
void Identify_Response(uint8_t* packet);
void Device_Info_Response(uint8_t* packet);
bool connectToServer();
void UserInput();
void initQueue();
void enqueue(struct dataPacket newData);
void readQueue();
void sendingPacket(boolean direction, int speed, int delayTime);
void setTimer(int time);
int getDelayTime(int speed);
void setDelayTime(char *args[10], int argc);
void viewDelayTime();
BLEScan* pBLEScan;

ARDUINO_ISR_ATTR void timerCallback(){
  stopflag = true;
}

void setup() {
  Serial.begin(115200); // baud rate = 115,200
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init(""); // BLE Device 초기 설정 -> 이름 지정 가능

  pBLEScan = BLEDevice::getScan(); // Scan을 위한 객체 생성
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks()); // 광고 패킷을 발견할 때 호출할 Callback함수 설정
  pBLEScan->setInterval(1349); // Interveal 설정 - 스캔의 간격을 설정한다
  pBLEScan->setWindow(449); // Window 설정 - 스캔마다 블루투스 장치를 수신하는 시간
  pBLEScan->setActiveScan(true); // 활성스캔 활성화 - 주변 모든 블루투스 기기 탐색
  pBLEScan->start(5, false);  // 5초간 Scan 시작

}

/* ----- Callback Function Start ----- */



// Device로부터 패킷이 날라올 때 호출되는 Callback 함수
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteNotification,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    // Check ID & Function Call
    if(pData[1] == 0xD0) Identify_Response(pData); // ID=0xD0 이면 Identify Response 패킷
    else if(pData[1] == 0xD1) Device_Info_Response(pData); // ID=0xD1 이면 Device Info Response 패킷
}

void MyClientCallback::onConnect(BLEClient* pclient){
  Serial.println("OnConnect");
}

void MyClientCallback::onDisconnect(BLEClient* pclient){
    connected = false;
    Serial.println("onDisconnect");
    initQueue();
    doScan = true;
}

// 원하는 Service UUID를 찾았을 때 
void MyAdvertisedDeviceCallbacks::onResult(BLEAdvertisedDevice advertisedDevice) {
    // 수신된 장치가 ServiceUUID를 가지고 이 UUID가 설정한 UUID와 같을 때 처리 구간
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

        BLEDevice::getScan()->stop(); // 스캔 중지
        myDevice = new BLEAdvertisedDevice(advertisedDevice); // 광고된 장치로 새로운 BLEAdvertisedDevice 객체 생성
        doConnect = true; // doConnect flag를 true로 설정하여 loop()에서 connectToServer() 함수 호출
        doScan = true;
    }
    else doScan = true;
}

/* ----- Callback Function End ----- */

/* ----- Delay Time Function Start -----*/

void setTimer(int time) {
  // 원하는 시간 뒤에 Timer Interrupt 발생 (Stop을 위해)
  timer = timerBegin(1000000);
  timerAttachInterrupt(timer, &timerCallback);
  timerAlarm(timer, time * 1000 , false, 0);
}

int getDelayTime(int speed){
    // 속도에 따라 설정된 duration(delay time) 반환
    int delayTime;
    switch(speed) {
    case 0:
      delayTime = delay_time[0];
      break;
    case 1:
      delayTime = delay_time[1];
      break;
    case 2:
      delayTime = delay_time[2];
      break;
    case 3:
      delayTime = delay_time[3];
      break;
    case 4:
      delayTime = delay_time[4];
      break;
    case 5:
      delayTime = delay_time[5];
      break;
    case 6:
      delayTime = delay_time[6];
      break;
    default:
      delayTime = delay_time[4];
      break;
  }
  return delayTime;
}

void setDelayTime(char *args[10], int argc){
  // 특정 속도의 duration 재설정
  if(argc != 2){
    Serial.println("Invalid parameter");
    return ;
  }
  if(atoi(args[0]) && atoi(args[0]) >= 0 && atoi(args[0]) <= 6){
    if(atoi(args[1]) && atoi(args[1]) >= 0){
      delay_time[atoi(args[0])] = atoi(args[1]);
      Serial.print("set Speed "); Serial.print(atoi(args[0])); Serial.print(" - "); Serial.print(args[1]); Serial.println("ms");
    }
    else{
      Serial.println("Input Correct duration time (over 0)");
      return ;
    }
  }
  else{
    Serial.println("Input Correct Speed 0~6");
    return ;
  }

}

void viewDelayTime(){
  // time table 출력
  Serial.println("----Time Table----");
  for(int i = 0 ; i < 7; i++){
    Serial.print("Speed ");
    Serial.print(i);
    Serial.print(" - ");
    Serial.print(delay_time[i]);
    Serial.println("ms");
  }
  Serial.println("------------------");
}

/* ----- Delay Time Function End -----*/

/* ----- Queue Function Start -----*/

void initQueue(){
  // Stop 또는 새로운 명령어 실행 시 작업 Queue를 초기화하기 위한 함수
    struct QueueNode* current = header;
    while (current != NULL) {
        struct QueueNode* temp = current;
        current = current->next;
        free(temp); // 노드 메모리 해제
    }
    header = tail = NULL; // header와 tail 포인터 초기화
}

// 새로운 패킷노드 삽입
void enqueue(struct dataPacket newData){
  struct QueueNode* newNode = (struct QueueNode*)malloc(sizeof(struct QueueNode));
  if (newNode == NULL) {
    // 메모리 할당 실패 시 처리
    Serial.println("Failed to allocate memory for new node");
    return;
  }
  newNode->data = newData;
  newNode->next = NULL;

  // 연결 리스트에 패킷 추가
  if(header == NULL){
    header = tail = newNode;
  }
  else{
    tail->next = newNode;
    tail = newNode;
  }
}

void readQueue(){
  // 큐에서 새로운 task를 추출하여 실행
  if(header == NULL){
    Serial.println("Empty taskQueue");
    return ;
  }

  // Queue 재배치
  struct QueueNode* temp = header;
  struct dataPacket usingData = temp->data;
  header = header->next;
  free(temp);
  if(header == NULL){
    lastTaskFlag = true;
    tail = NULL;
  }

  // 패킷 전송 함수 호출
  sendingPacket(usingData.direction, usingData.speed, usingData.delayTime);
}

// 패킷 전송 Process
void sendingPacket(boolean direction, int speed, int delayTime){
  // grip process
  if(direction){
    pRemoteWrite->writeValue(Packet_Table[1][speed], 7);
    if(speed != 0) {
      Serial.print("grip  Speed "); Serial.print(speed); Serial.println("...");
    }
    setTimer(delayTime);
    workingFlag = true;
  }
  else{
    pRemoteWrite->writeValue(Packet_Table[0][speed], 7);
    if(speed != 0) {
      Serial.print("release  Speed "); Serial.print(speed); Serial.println("...");
    }
    setTimer(delayTime);
    workingFlag = true;
  }
}
/* ----- Queue Function End -----*/

/* ----- Request Function Start -----*/

void grip(char *args[10], int argc){
  int speed;
  initQueue(); // task Queue 초기화
  if(workingFlag){
    lastTaskFlag = false;
    stop();
  }
  if(argc >= 2){
    Serial.println("Invalid parameter");
    return ;
  }
  // 사용자가 속도 설정을 하지 않았을 때, 4로 고정
  if(argc == 0) speed = 4;
  else {
    if(atoi(args[0]) && atoi(args[0]) >= 0 && atoi(args[0]) <= 6){
      speed = atoi(args[0]);
    }
    else{
      Serial.println("Input Correct Speed 0~6");
      return ;
    }
  }
  int delayTime = getDelayTime(speed);
  struct dataPacket sendData = {true, speed, delayTime};
  // taskQueue에 sendData 추가
  enqueue(sendData);
}


void release(char *args[10], int argc){
  int speed;
  initQueue(); // task Queue 초기화
  if(workingFlag){
    lastTaskFlag = false;
    stop();
  }
  if(argc >= 2){
    Serial.println("Invalid parameter");
    return ;
  }
  // 사용자가 속도 설정을 하지 않았을 때, 4로 고정
  if(argc == 0) speed = 4;
  else {
    if(atoi(args[0]) && atoi(args[0]) >= 0 && atoi(args[0]) <= 6){
      speed = atoi(args[0]);
    }
    else{
      Serial.println("Input Correct Speed 0~6");
      return ;
    }
  }
  int delayTime = getDelayTime(speed);
  struct dataPacket sendData = {false, speed, delayTime};
  // taskQueue에 sendData 추가
  enqueue(sendData);
}

void griprelease(char *args[10], int argc){
  int speed;
  int count;
  initQueue(); // task Queue 초기화
  // 이전 작업 동작중이면 stop으로 설정
  if(workingFlag){
    lastTaskFlag = false;
    stop();
  }
  if(argc >= 3){
    Serial.println("Invalid Parameter");
    return ;
  }
  // 사용자가 아무 파라미터 입력하지 않았을 때, 속도 4로 1회 반복
  if(argc == 0){
    speed = 4;
    count = 1;
  }
  // 사용자가 속도 설정을 하지 않았을 때, 4로 고정
  else if(argc == 1){
    speed = atoi(args[0]);
    count = 1;
  }
  else{
    if(atoi(args[0]) && atoi(args[0]) >= 0 && atoi(args[0]) <= 6){
      speed = atoi(args[0]);
    }
    else{
      Serial.println("Input Correct Speed 0~6");
      return ;
    }
    if(atoi(args[1]) && atoi(args[1]) > 0){
      if(atoi(args[1]) > 20) count = 20;
      else{
        count = atoi(args[1]);
      }      
    }
    else{
      Serial.println("Input Correct Repeat Count(Over 0)");
    }
  }
  int delayTime = getDelayTime(speed);

  struct dataPacket sendData = {true, speed, delayTime};
  struct dataPacket emptyData = {true, 0, delay_time[0]};
  // taskQueue에 sendData 추가
  for(int i = 0; i<count; i++){
    // grip, release 반복 간에 지연 시간을 주기 위해 속도 0인 패킷을 추가
    sendData.direction = true;
    enqueue(sendData);
    enqueue(emptyData);
    sendData.direction = false;
    enqueue(sendData);
    enqueue(emptyData);
  }
}

void releasegrip(char *args[10], int argc){
  int speed;
  int count;
  initQueue(); // task Queue 초기화
  if(workingFlag){
    lastTaskFlag = false;
    stop();
  }
  if(argc >= 3){
    Serial.println("Invalid Parameter");
    return ;
  }
  // 사용자가 매개변수을 입력하지 않았을 때, 속도 4로 1회 반복
  if(argc == 0){
    speed = 4;
    count = 1;
  }
  // 사용자가 매개변수를 1개만 입력했을 때, 해당 속도로 1회 반복
  else if(argc == 1){
    speed = atoi(args[0]);
    count = 1;
  }
  // 사용자가 매개변수를 2개 입력했을 때, 해당 속도로 해당 Count만큼 반복
  else{
    if(atoi(args[0]) && atoi(args[0]) >= 0 && atoi(args[0]) <= 6){
      speed = atoi(args[0]);
    }
    else{
      Serial.println("Input Correct Speed 0~6");
      return ;
    }
    if(atoi(args[1]) && atoi(args[1]) > 0){
      if(atoi(args[1]) > 20) count = 20;
      else{
        count = atoi(args[1]);
      }  
    }
    else{
      Serial.println("Input Correct Repeat Count(Over 0)");
    }
  }
  int delayTime = getDelayTime(speed);
  struct dataPacket sendData = {false, speed, delayTime};
  struct dataPacket emptyData = {true, 0, delay_time[0]};
  // taskQueue에 sendData 추가
  for(int i = 0; i<count; i++){
    // grip, release 반복 간에 지연 시간을 주기 위해 속도 0인 패킷을 추가
    sendData.direction = false;
    enqueue(sendData);
    enqueue(emptyData);
    sendData.direction = true;
    enqueue(sendData);
    enqueue(emptyData);
  }
}

// Sensor Data Packet 수신을 정지하는 packet 전송
void stop(){
  if(lastTaskFlag){
    Serial.println("task complete");
    lastTaskFlag = false;
  }
  pRemoteWrite->writeValue(Sensor_Stop, 5);
  if(timer != NULL){
    timerEnd(timer);
    timer = NULL;
  }
  workingFlag = false;
}

// Product ID 및 Version을 요청하는 packet 전송
void identify(){
  pRemoteWrite->writeValue(Identify_Request, 5);
}

// 디바이스 info를 요청하는 packet 전송
void deviceinfo(){
  pRemoteWrite->writeValue(Device_Info_Request, 5);
}

/* ----- Request Function Start -----*/

/* ----- Response Function Start ----- */

// 패킷의 아이디가 0xD0 -> Identify Response Packet 처리
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

// 패킷의 아이디가 0xD1 -> Device Info Response Packet 처리
void Device_Info_Response(uint8_t* packet){
  if(packet[3] == 1) Serial.println("Battery High");
  else if(packet[3] == 0) Serial.println("Battery Low");
}

/* ----- Response Function End ----- */


// Connect to device & Find service and add characteristic(write, notification)
bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient(); // 연결을 위한 새로운 Client 객체 생성
    Serial.println(" - Created client");
 
    pClient->setClientCallbacks(new MyClientCallback()); // 생성된 Client에 Callback함수 설정(Connect, Disconnect)

    pClient->connect(myDevice);  // myDevice에 Connect 시도
    Serial.println(" - Connected to server");
    pClient->setMTU(517); 

    // Find Service
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID); // Client에서 device의 서비스 가져오기
    if (pRemoteService == nullptr) {
      // 서비스가 존재하지 않는다면, 연결 해제
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our Service");


    // characteristic(write) 불러오기
    pRemoteWrite = pRemoteService->getCharacteristic(writeUUID);
    if (pRemoteWrite == nullptr) {
      // Service에 정의된 UUID를 가지는 characteristic이 존재하지 않는다면 연결 해제
      Serial.print("Failed to find our Write UUID: ");
      Serial.println(writeUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our Write");

    // characteristic(notification) 불러오기
    pRemoteNotification = pRemoteService->getCharacteristic(notificationUUID);
    if (pRemoteNotification == nullptr) {
      // Service에 정의된 UUID를 가지는 characteristic이 존재하지 않는다면 연결 해제
      Serial.print("Failed to find our Notification UUID: ");
      Serial.println(notificationUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our Notification");

    // Characteristic으로 부터 데이터를 수신받았을 때 Callback 함수 설정
    if(pRemoteWrite->canNotify())
      pRemoteWrite->registerForNotify(notifyCallback);

    if(pRemoteNotification->canNotify())
      pRemoteNotification->registerForNotify(notifyCallback);

    // 처음 연결이 되었을 때, 기기의 통신 상태 및 버전, 배터리 확인을 위해 identify, device info reqeust 전송
    identify(); delay(1000);
    deviceinfo(); delay(1000);

    connected = true;
    return true;
}



// 사용자로부터 Serial input이 들어왔을 때 호출되는 함수
void UserInput(){
  char *charData, *args[10], *command, *token = NULL;
  int argc = 0;
  String inputData = Serial.readStringUntil('\n'); // 줄바꿈 문자가 입력될때까지 Serial read
  if(!connected){
    Serial.println("Not Connected (try again after connect device)");
    return ;
  }
  charData = (char *)inputData.c_str(); //strtok() 사용을 위해 String -> char * 변경
  token = strtok(charData, " \n"); command = token; //strtok로 공백, 줄바꿈 기준으로 parsing 시작
  // 최대 10개까지 매개변수를 args에 저장
  while(token != NULL){
    if(argc > 10) break;
    token = strtok(NULL, " \n");
    args[argc] = token;
    argc++;
  }
  argc--;
  // 0번째 입력 단어(command)에 따라 각기 다른 함수를 호출해 Device에게 packet 전송
  if(strcmp(command, "grip") == 0) grip(args, argc);
  else if(strcmp(command, "release") == 0) release(args, argc);
  else if(strcmp(command, "griprelease") == 0) griprelease(args, argc);
  else if(strcmp(command, "releasegrip") == 0) releasegrip(args, argc);
  else if(strcmp(command, "stop") == 0) {
    lastTaskFlag = false;
    stop();
    Serial.println("stop");
    initQueue();
  }
  else if(strcmp(command, "identify") == 0) identify();
  else if(strcmp(command, "deviceinfo") == 0) deviceinfo();
  else if(strcmp(command, "time") == 0) setDelayTime(args, argc);
  else if(strcmp(command, "timetable") == 0) viewDelayTime();
  else{
    Serial.print(command); Serial.println(" is invaild Command");
  }
  
}

void loop() {
  if(stopflag){
    stop();
    stopflag = false;
  }
  // 해당 부분은 일치하는 serviceUUID를 찾았을 때 doConnect가 true로 설정되어 한번만 호출
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false; // 1회성 호출임
  }


  if(doScan && !connected){
    BLEDevice::getScan()->start(5, false);
  }

  if(header != NULL && !workingFlag){
    readQueue();
  }

  // 사용자 입력이 있을 때 UserInput 함수 호출
  if(Serial.available()){
    UserInput();
  }
  delay(1);
} 