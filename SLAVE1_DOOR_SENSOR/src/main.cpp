#define DEBUG_ENABLE

#define ARDUINO_UNO
// #define ARDUINO_NANO
// #define ESP8266

/*******************************************************************/

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Arduino_FreeRTOS.h>

#if defined(ARDUINO_UNO) || defined(ARDUINO_NANO)
#define CE 9
#define CSN 10
#define DOOR_SENSOR A1
#elif defined(ESP8266)
#define CE 6
#define CSN 7
#endif

#if defined DEBUG_ENABLE
#define PRINT(x) Serial.print(x);
#define PRINTLN(x) Serial.println(x);
#else
#define PRINT(x) ;
#define PRINTLN(x) ;
#endif

RF24 radio(CE, CSN);  // CE, CSN

const byte myAddress[6] = "10001";      // Common TX
const byte masterAddress[6] = "10000";  // Common RX
const int myId = 10001;

void send_Task(void *pvParameters);
void doorSensor_Task(void *pvParameters);

TaskHandle_t sendTask_Handle;

bool doorState = false;

struct slave1{
 int id;
 int door;
};

const char* firmwareVersion = "Rev_01";

void setup() {
  Serial.begin(9600);
  Serial.println("Firmware Version:" + String(firmwareVersion));

#if defined DEBUG_ENABLE
  Serial.println("Debug Enabled.");
#else
  Serial.println("Debug Disabled.");
#endif

  pinMode(DOOR_SENSOR, INPUT_PULLUP);
  doorState = (digitalRead(DOOR_SENSOR) == HIGH);  // Initialize door state
  PRINTLN("Door State:" + String(doorState));

  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.openWritingPipe(masterAddress);  // Address to write to
  radio.openReadingPipe(1, myAddress);   // Address to read from

  // Create the FreeRTOS task
  xTaskCreate(send_Task, "SendTask", 512, NULL, 2, &sendTask_Handle);
  xTaskCreate(doorSensor_Task, "doorSensorTask", 255, NULL, 1, NULL);
  
  // vTaskSuspend(sendTask_Handle);
}

void loop() {
  // Main loop is empty as tasks handle everything
}

void doorSensor_Task(void *pvParameters) {
  while (1) {
    // Read the door sensor state
    if (digitalRead(DOOR_SENSOR) == HIGH) {
      doorState = true;
      // vTaskResume(sendTask_Handle); // Resume send task when door is open
    } else {
      doorState = false;
      // vTaskSuspend(sendTask_Handle); // Suspend send task when door is closed
      // PRINTLN("Door Closed!");
    }

    // Small delay to avoid task hogging CPU
    vTaskDelay(50 / portTICK_PERIOD_MS); // Increased delay for stability
  }
}

void send_Task(void *pvParameters) {

  while (1) {

    radio.stopListening();     // Stop listening to send data
    
    // int sendData = doorState;  // Data to send
    struct slave1 sendSlave1Data;
    sendSlave1Data.id = myId;
    sendSlave1Data.door = doorState;

    bool result = radio.write(&sendSlave1Data, sizeof(sendSlave1Data));
    if (result) {
      PRINT("Sending =>");
      PRINT("ID:"); PRINT(sendSlave1Data.id);
      PRINT(", Door:"); PRINT(sendSlave1Data.door);
      PRINT(", Result: ");
      PRINTLN("Success");
    } else {
      PRINTLN("Sending Failed or Master disconnected!");
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
