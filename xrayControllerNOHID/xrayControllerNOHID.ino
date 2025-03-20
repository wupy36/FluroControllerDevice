// ESP32 s3 Fluoroscopy Demo NO HID IMPLEMENTATION
// Written by Chase Theiler
// Written for use by Mayo Clinic, specifically the Anatomical Modeling Lab.
// chasetheiler@gmail.com or theil027@umn.edu for questions.

// Functions as a Fluoroscopy Control Panel, where multiple joystick and button inputs are broadcasted over bluetooth enabling low-latency wireless communication.
// Once connected, the data within the custom service 1848 and characteristic 03C4 can be read by the client device (ex. VR Headset, Phone, ETC.)

// Uses the ESP32-S3 module as a bluetooth input device, with a custom Bluetooth Low Energy service (1848) and characteristic (03C4) to display large amounts of input data for computer interfacing.
// Has 10 analog joystick inputs (2 axes for each joystick), uses MCP23017 IO Expanders as modular button input banks.

// Has half-implemented ESP-NOW functionality, enabling a secondary control module to relay inputs to this one over WiFi. ** NOT WORKING, MAY REIMPLEMENT WITH BLUETOOTH

// Pins
// [0]      [1]   [2]    [3]   [4]   [5]    [6]   [7]   [8]   [9]   [10]
// NC     sDown  axis   axis  sDown  axis  axis  sDown  axis  axis sDown ... cont until/including pin 15

#include <Arduino.h>
#include "BLEDevice.h"
//#include <esp_now.h>
//#include <WiFi.h>
#include "MCP23017.h"

#define DEVICE_NAME "Fluoro Sim Controls"


uint8_t CONNECTED_LED = 26;
uint8_t ADVERTISING_LED = 21;
uint8_t SDA_PIN = 33;
uint8_t SCL_PIN = 34;

typedef struct struct_message {
  uint32_t secretInt;
  uint32_t extraData;
} struct_message;
struct_message recieverData;

void bluetoothTask(void*);
void sendTestReport(uint8_t*, uint8_t*);

uint8_t joystick_values[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
uint8_t joystick_down[5] = { 0, 0, 0, 0, 0 };
uint8_t portA = 0x0;
uint8_t portB = 0x0;
uint8_t externButtons = 0x0;
bool isBleConnected = false;
bool disableNormal = true;
bool joysticksConn = true;

MCP23017 mcp = MCP23017(0x24);

void DataRecieveOtherDevice(const uint8_t* mac, const uint8_t* incomingData, int len) {  // USED FOR RECIEVING DATA FROM ESP-NOW OTHER DEVICE
  memcpy(&recieverData, incomingData, sizeof(recieverData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.println(recieverData.secretInt, BIN);
  Serial.println(recieverData.extraData, BIN);
}

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  mcp.init();
  Serial.begin(115200);
  pinMode(CONNECTED_LED, OUTPUT);
  pinMode(ADVERTISING_LED, OUTPUT);
  pinMode(1, INPUT_PULLUP);  // Stick 1 pushdown

  // ESP-NOW Functionality
  // WiFi.mode(WIFI_STA);
  // Init ESP-NOW
  //if (esp_now_init() != ESP_OK) {
  //Serial.println("Error initializing ESP-NOW");
  //}
  // Register reciever callback (cb) function
  // esp_now_register_recv_cb(DataRecieveOtherDevice);
  mcp.writeRegister(MCP23017Register::IPOL_A, (uint8_t)0xFF);  // input polarity register A, inverts all input polarities.
  mcp.writeRegister(MCP23017Register::IPOL_B, (uint8_t)0xFF);  // input polarity register B

  //externGPIO = (mcp.readRegister(MCP23017Register::GPIO_A) << 8) | mcp.readRegister(MCP23017Register::GPIO_B);

  // start Bluetooth task
  xTaskCreate(bluetoothTask, "bluetooth", 20000, NULL, 5, NULL);

  delay(3000);
}

// Reads Joystick Inputs from pins 1-15.
void readAnalogValues() {  // assigns values starting at pin 1, in a [down, stickY, stickX] order. (probably really hard to follow, sorry. It is a geometric pattern.)
  for (int i = 0; i < 5; i++) {
    joystick_down[i] = digitalRead(1 + i + i + i);                // 3*i + 1, where stickDown inputs are 1, 4, 7, 10, 13
    joystick_values[i + i] = analogRead(2 + i + i + i) / 16;      // sets JOYSTICK Y
    joystick_values[i + i + 1] = analogRead(3 + i + i + i) / 16;  // sets JOYSTICK X
  }
}

void loop() {
  if (isBleConnected) {
    portB = (uint8_t)mcp.readRegister(MCP23017Register::GPIO_B);
    portA = (uint8_t)mcp.readRegister(MCP23017Register::GPIO_A);
    //externGPIO = (mcp.readRegister(MCP23017Register::GPIO_A) << 8) | mcp.readRegister(MCP23017Register::GPIO_B);
    readAnalogValues();  // reads any connected joystick values.
    sendTestReport(&portA, &portB);
    delay(10);
  }
}

struct NEW_GamepadReport {
  uint8_t x1;
  uint8_t y1;

  uint8_t x2;
  uint8_t y2;

  uint8_t x3;
  uint8_t y3;

  uint8_t x4;
  uint8_t y4;

  uint8_t x5;
  uint8_t y5;

  uint8_t buttons1;
  uint8_t buttons2;
  uint8_t buttons3;
  uint8_t buttons4;

  uint8_t joysticksDown;
  uint8_t extra2;

  uint32_t large1;
  uint32_t large2;
  uint32_t large3;
  uint32_t large4;
  uint32_t large5;
  uint32_t large6;
};

//BLEHIDDevice* hid;
//BLECharacteristic* input;
//BLECharacteristic* output;
BLECharacteristic* pChar1;
BLEAdvertising* advertising;

// Callbacks for BLE
class BleControllerCallbacks : public BLEServerCallbacks {

  void onConnect(BLEServer* server) {
    isBleConnected = true;

    // Allow notifications for characteristics
    //BLE2902* cccDesc = (BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902)); // "Client Characteristic Configuration"
    //cccDesc->setNotifications(true);

    Serial.println("Client has connected");
    digitalWrite(ADVERTISING_LED, LOW);
    digitalWrite(CONNECTED_LED, HIGH);
  }

  void onDisconnect(BLEServer* server) {
    isBleConnected = false;

    // Disallow notifications for characteristics
    //BLE2902* cccDesc = (BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    //cccDesc->setNotifications(false);

    Serial.println("Client has disconnected");
    digitalWrite(CONNECTED_LED, LOW);
    delay(20);
    abort();  // RESTARTS THE PROGRAM
  }
};

void bluetoothTask(void*) {

  // initialize the device
  BLEDevice::init(DEVICE_NAME);

  BLEServer* server = BLEDevice::createServer();       // this is a server, the receiving end is the client.
  server->setCallbacks(new BleControllerCallbacks());  // not used atm

  // create an HID device
  //hid = new BLEHIDDevice(server); // creates a HID device on the server
  //input = hid->inputReport(1); // report ID

  // RESEARCH THIS ^^^^^
  //output = hid->outputReport(1); // report ID
  //output->setCallbacks(new OutputCallbacks());

  // set manufacturer name
  //hid->manufacturer()->setValue("Theiler");
  // set USB vendor and product ID
  //hid->pnp(0x02, 0x21BF, 0xa111, 0x0210); // sig, vid, pid, version

  // information about HID device: device is not localized, device can be connected
  //hid->hidInfo(0x00, 0x02);

  // Security: device requires bonding
  BLESecurity* security = new BLESecurity();
  security->setAuthenticationMode(ESP_LE_AUTH_BOND);

  // set report map
  //hid->reportMap((uint8_t*)NEW_GAMEPAD_DESCRIPTOR, sizeof(NEW_GAMEPAD_DESCRIPTOR));
  //hid->startServices();

  // set battery level to 100%
  //hid->setBatteryLevel(100);

  // advertise the services
  advertising = server->getAdvertising();
  advertising->setAppearance(964);  // HID_GAMEPAD, (HID implementation was taken out of this project)
  //advertising->addServiceUUID(hid->hidService()->getUUID());
  //advertising->addServiceUUID(hid->deviceInfo()->getUUID());
  //advertising->addServiceUUID(hid->batteryService()->getUUID());

  // possibly create a write-only characteristic that can relay data back to the sim module.
  //  * characteristic within 1848

  // creates a custom service and characteristic with everything unlocked about it (read/write/notify).
  // service: 1848, characteristic: 03C4
  BLEService* pService1 = server->createService(BLEUUID((uint16_t)0x1848));  // creates a custom service with uuid 0x1848
  pChar1 = pService1->createCharacteristic(BLEUUID((uint16_t)0x3c4), BLECharacteristic::PROPERTY_NOTIFY);
  pChar1->setReadProperty(true);
  pChar1->setWriteProperty(true);
  pChar1->setWriteNoResponseProperty(true);
  BLEDescriptor* pChar1Desc = new BLEDescriptor(BLEUUID((uint16_t)0x2902));
  pChar1->addDescriptor(pChar1Desc);
  pChar1->setBroadcastProperty(true);

  pService1->start();
  advertising->addServiceUUID(BLEUUID((uint16_t)0x1848));
  begin_advertising();
};

void begin_advertising() {
  advertising->start();
  digitalWrite(ADVERTISING_LED, HIGH);  // indicates advertising

  Serial.println("BLE advertising");
  delay(portMAX_DELAY);
}


void IRAM_ATTR sendTestReport(uint8_t* portA, uint8_t* portB) {  // broadcasts the input states, updating the characteristic 03C4's data in service 1848.
  NEW_GamepadReport new_game_report = {
    .x1 = joystick_values[0],
    .y1 = joystick_values[1],
    .x2 = joystick_values[2],
    .y2 = joystick_values[3],
    .x3 = joystick_values[4],
    .y3 = joystick_values[5],
    .x4 = joystick_values[6],
    .y4 = joystick_values[7],
    .x5 = joystick_values[8],
    .y5 = joystick_values[9],  // 10 joystick bytes

    .buttons1 = (uint8_t)*portA,  // MCP port A
    .buttons2 = (uint8_t)*portB,  // MCP port B
    .buttons3 = 0x0,
    .buttons4 = 0x0,

    .joysticksDown = 0x0,
    .extra2 = 0xEF,

    .large1 = 0xF9F9F9F9,
    .large2 = 0xFAFAFAFA,
    .large3 = 0xFBFBFBFB,
    .large4 = 0xFCFCFCFC,
    .large5 = 0xFDFDFDFD,
    .large6 = 0xFEFEFEFE
  };
  pChar1->setValue((uint8_t*)&new_game_report, sizeof(new_game_report));
  pChar1->notify();
  delay(5);
}
