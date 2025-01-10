#define DEBUG_ENABLE

#define ARDUINO_UNO
// #define ARDUINO_NANO
// #define ESP8266

/********************************************************************/

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Arduino_FreeRTOS.h>

#if defined(ARDUINO_UNO) || defined(ARDUINO_NANO)
#define CE 9
#define CSN 10
#define DOOR_LED A0
#define SLAVE1_LED 2
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

// Define addresses (must be 3-5 bytes)
const byte myAddress[6] = "10000";      // Common TX
const byte slave1Address[6] = "10001";  // Door sensor RX

void Slave1_Task(void *pvParameters);
void Process_Task(void *pvParameters);

bool doorState = false;
bool slave1_isConnected = false;

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

  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.openWritingPipe(slave1Address);  // Address to write to
  radio.openReadingPipe(1, myAddress);   // Address to read from

  // Create the FreeRTOS task
  xTaskCreate(Slave1_Task, "Slave1_Task", 512, NULL, 2, NULL);
  xTaskCreate(Process_Task, "Process_Task", 255, NULL, 1, NULL);
}

void loop() {
  // Main loop is empty as tasks handle everything
}

void Process_Task(void *pvParameters) {
  pinMode(DOOR_LED, OUTPUT);
  pinMode(SLAVE1_LED, OUTPUT);

  while (1) {
    if (doorState) {
      digitalWrite(DOOR_LED, HIGH);
      // PRINTLN("Led ON");
    } else {
      digitalWrite(DOOR_LED, LOW);
      // PRINTLN("Led OFF");
    }

    if (slave1_isConnected) {
      digitalWrite(SLAVE1_LED, HIGH);
      // PRINTLN("Slave1 Connected");
    } else {
      digitalWrite(SLAVE1_LED, LOW);
      // PRINTLN("Slave1 Disconnected");
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void Slave1_Task(void *pvParameters) {

  while (1) {

    // Start listening for a response
    radio.startListening();

    // Wait for a response with a timeout
    unsigned long startTime = millis();
    bool timeout = false;

    while (!radio.available()) {
      if (millis() - startTime > 500) {  // Timeout after 200ms
        timeout = true;
        break;
      }
    }

    if (!timeout) {
      // int receiveData = 0;
      struct slave1 receiveSlave1Data;

      radio.read(&receiveSlave1Data, sizeof(receiveSlave1Data));  // Read incoming data
      PRINT("Receiving =>");
      PRINT("ID:")
      PRINT(receiveSlave1Data.id);
      PRINT(", Door:")
      PRINT(receiveSlave1Data.door);
      PRINT(", Result: Door");
      PRINTLN((receiveSlave1Data.door == 1) ? "[Open]" : "[Close]");

       if (receiveSlave1Data.door == 1) {
        doorState = true;
      }else{
        doorState = false;
      }
      // If data received, delay before next cycle
      if (receiveSlave1Data.id != 0) {
        slave1_isConnected = true;
        vTaskDelay(300 / portTICK_PERIOD_MS);
      }
    } else {
      PRINTLN("Slave1 Disconnected or powered Off, Timeout");
      slave1_isConnected = false;
    }

    // Delay before the next iteration
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}
