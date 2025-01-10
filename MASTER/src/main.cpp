#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Arduino_FreeRTOS.h>

#define SLAVE1_ID 10001

RF24 radio(9, 10);                     // CE, CSN
const byte myAddress[6] = "10000";     // MASTER
const byte slave1Address[6] = "10001"; // SLAVE1

struct dataa
{
  int slave_id;
  int count;
  char text[10];
};

void nrf24_init()
{
  radio.begin();
  radio.openWritingPipe(slave1Address); // sending to slave1
  radio.openReadingPipe(1, myAddress);  // receive from master
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
}

void receive_task(void *pvParameters)
{

  while(1){
  radio.startListening();
  if (radio.available())
  {

    struct dataa receiveData;

    // char text[32] = "";//print only 32 characters
    radio.read(&receiveData, sizeof(receiveData));
      Serial.print("SLAVE1 => ");
      Serial.print("id:");
      Serial.print(receiveData.slave_id);
      Serial.print(", count:");
      Serial.print(receiveData.count);
      Serial.print(", text:");
      Serial.println(receiveData.text);

  }
  vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void send_task(void *pvParameters)
{
  char sendText[] = "madhan";

  while (1)
  {

    bool success = radio.write(&sendText, sizeof(sendText));
    if (!success)
    {
      Serial.println("Send Success.[" + String(sendText) + "]");
    }
    else
    {
      Serial.println("Send Failed.");
    }

    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(9600);

  nrf24_init();

  xTaskCreate(send_task, "send task", 255, NULL, 1, NULL);
  xTaskCreate(receive_task, "receive task", 255, NULL, 1, NULL);
  
}

void loop()
{


}
