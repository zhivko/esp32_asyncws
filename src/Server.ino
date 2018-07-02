// platformio run --target uploadfs

#include <WiFi.h>
#include <FS.h>
#include "SPIFFS.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPI.h>
#include <ArduinoOTA.h>
#include <stdint.h>


const char* ssid = "AndroidAP";
const char* password =  "Doitman1";
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws


static const int spiClk = 1000000; // 1 MHz
uint16_t toTransfer;
//uninitalised pointers to SPI objects
SPIClass * vspi = NULL;

// use 13 bit precission for LEDC timer
#define LEDC_TIMER_10_BIT  10
// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ 5000
// fade LED PIN (replace with LED_BUILTIN constant for built-in LED)
#define PWM1_PIN 12
#define PWM2_PIN 14
#define PWM3_PIN 26
#define PWM4_PIN 27
// use first channel of 16 channels (started from zero)
#define LEDC_CHANNEL_0 0
#define LEDC_CHANNEL_1 1
#define LEDC_CHANNEL_2 2
#define LEDC_CHANNEL_3 3
#define SS1 33
#define pwmValueMax  2000

int brightness = 0; // how bright the LED is
int fadeAmount = 1; // how many points to fade the LED by

String status1;
String status2;


void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 1024) {
  // calculate duty, 8191 from 2 ^ 13 - 1
  uint32_t duty = (8191 / valueMax) * _min(value, valueMax);

  // write duty to LEDC
  ledcWrite(channel, duty);
}


void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void testSpi()
{
  //SPI_MODE0, ..., SPI_MODE3
  vspi->setDataMode(SPI_MODE1);
  vspi->setHwCs(false);

  usleep(1);
  digitalWrite(SS1, LOW);
  // SPI WRITE
  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE1));

  // http://www.ti.com/product/drv8702-q1?qgpn=drv8702-q1
  // datasheet: http://www.ti.com/lit/gpn/drv8702-q1
  // page 42


  byte data_read = B10000000;  // READ OPERATION
  byte data_address = B00010000; // ADDRES 02x MAIN REGISTER
  byte data = data_read | data_address;
  byte lowbyte = B0;
  uint16_t data_int = data << 8 | lowbyte;

  uint16_t reply = vspi->transfer16(data_int);  // should return 0x18 B00011000
  vspi->endTransaction();
  digitalWrite(SS1, HIGH);
  usleep(1);
  if(reply == B00011000)
  {
    status1 = "DRV8703Q is ON (Not locked).";
  }

  usleep(1);
  digitalWrite(SS1, LOW);
  // SPI WRITE
  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE1));
  // fault bytes:
  uint8_t FAULT_FAULT = B1 << 7;     // FAULT R 0b Logic OR of the FAULT status register excluding the OTW bit
  uint8_t FAULT_WDFLT = B1 << 6;     // WDFLT R 0b Watchdog time-out fault
  uint8_t FAULT_GDF = B1 << 5;       // GDF R 0b Indicates gate drive fault condition
  uint8_t FAULT_OCP = B1 << 4;       // OCP R 0b Indicates VDS monitor overcurrent fault condition
  uint8_t FAULT_VM_UVFL = B1 << 3;   // VM_UVFL R 0b Indicates VM undervoltage lockout fault condition
  uint8_t FAULT_VCP_UVFL = B1 << 2;  // VCP_UVFL R 0b Indicates charge-pump undervoltage fault condition
  uint8_t FAULT_OTSD = B1 << 1;      // OTSD R 0b Indicates overtemperature shutdown
  uint8_t FAULT_OTW = B1 << 0;       // OTW R 0b Indicates overtemperature warning

  data_read = B10000000;  // READ OPERATION
  data_address = B00000000; // ADDRES 0x FAULT REGISTER
  data = data_read | data_address;
  lowbyte = B0;
  data_int = data << 8 | lowbyte;

  reply = vspi->transfer16(data_int);  // should return 0x18
  vspi->endTransaction();
  digitalWrite(SS1, HIGH);
  usleep(1);
  //Serial.println(reply);

  if((reply & FAULT_FAULT) > 0)
  {
    //Serial.println("fault");
  }
  if((reply & FAULT_WDFLT) > 0)
  {
    status1 = "Watchdog time-out fault";
  }
  if((reply & FAULT_GDF) > 0)
  {
    status1 = "Gate drive fault";
  }
  if((reply & FAULT_OCP) > 0)
  {
    status1 = "VDS monitor overcurrent fault";
  }
  if((reply & FAULT_VM_UVFL) > 0)
  {
    status1 = "VM undervoltage lockout fault";
  }
  if((reply & FAULT_VCP_UVFL) > 0)
  {
    status1 = "Charge-pump undervoltage fault";
  }
  if((reply & FAULT_OTSD) > 0)
  {
    status1 = "Overtemperature shutdown";
  }
  if((reply & FAULT_OTW) > 0)
  {
    status1 = "Overtemperature warning";
  }
}

void pwmUp(){
  if(brightness<=(pwmValueMax-1))
  {
    brightness = brightness + 1;
    ledcAnalogWrite(LEDC_CHANNEL_0, brightness, pwmValueMax);
  }
}

void pwmDown()
{
  if(brightness>=1)
  {
   brightness = brightness - 1;
   ledcAnalogWrite(LEDC_CHANNEL_0, brightness, pwmValueMax);
  }
  Serial.print("duty= ");
  Serial.println(brightness);
}

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    //client connected
    printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    //client disconnected
    printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    //error was received from the other end
    printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    //pong message was received (in response to a ping request maybe)
    printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    //data packet
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    printf("Data came...");
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);
      if(info->opcode == WS_TEXT){
        printf("Data11: ");
        data[len] = 0;
        printf("%s\n", (char*)data);
      } else {
        for(size_t i=0; i < info->len; i++){
          printf("%02x ", data[i]);
        }
        printf("\n");
      }
      if(info->opcode == WS_TEXT)
        client->text("I got your text message");
      else
        client->binary("I got your binary message");
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0)
          printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);
      if(info->message_opcode == WS_TEXT){
        printf("%s\n", (char*)data);
      } else {
        for(size_t i=0; i < len; i++){
          printf("%02x ", data[i]);
        }
        printf("\n");
      }

      if((info->index + len) == info->len){
        printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}

void setup(){
  Serial.begin(115200);
  Serial.print("ESP ChipSize:");
  Serial.println(ESP.getFlashChipSize());

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println(WiFi.localIP());

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("/status");
    testSpi();
    request->send(200, "text/plain", status1);
  });
  server.on("/up", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("/up");
    pwmUp();
    request->send(200, "text/plain", "Hello World");
  });
  server.on("/down", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("/down");
    request->send(200, "text/plain", "Hello World");
  });

  if(!SPIFFS.begin()){
      Serial.println("SPIFFS Mount Failed");
      return;
  }

  listDir(SPIFFS, "/", 0);

  ws.onEvent(onEvent);
  server.addHandler(&ws);

  server.serveStatic("/", SPIFFS, "/");



  server.begin();
  ArduinoOTA.begin();
}

void loop(){
  ArduinoOTA.handle();
  sleep(1);
}
