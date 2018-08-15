# 1 "c:\\users\\klemen\\appdata\\local\\temp\\tmp07jgon"
#include <Arduino.h>
# 1 "C:/Users/klemen/Dropbox/Voga/esp32_asyncws/src/Server.ino"
# 11 "C:/Users/klemen/Dropbox/Voga/esp32_asyncws/src/Server.ino"
#include <WiFi.h>

#include <FS.h>

#include "SPIFFS.h"

#include <AsyncTCP.h>

#include <ESPAsyncWebServer.h>

#include <SPI.h>

#include <ArduinoOTA.h>

#include <stdint.h>



#define LED_PIN GPIO_NUM_2



String ssid = "AsusKZ";

String password = "Doitman1";



bool reportingJson=false;



char* softAP_ssid = "MLIFT";

char* softAP_password = "Doitman1";



AsyncWebServer server(80);

AsyncWebSocket ws("/ws");

const char * mysystem_event_names[] = { "WIFI_READY", "SCAN_DONE", "STA_START", "STA_STOP", "STA_CONNECTED", "STA_DISCONNECTED", "STA_AUTHMODE_CHANGE", "STA_GOT_IP", "STA_LOST_IP", "STA_WPS_ER_SUCCESS", "STA_WPS_ER_FAILED", "STA_WPS_ER_TIMEOUT", "STA_WPS_ER_PIN", "AP_START", "AP_STOP", "AP_STACONNECTED", "AP_STADISCONNECTED", "AP_PROBEREQRECVED", "GOT_IP6", "ETH_START", "ETH_STOP", "ETH_CONNECTED", "ETH_DISCONNECTED", "ETH_GOT_IP", "MAX"};







const char* NTP_SERVER0 = "0.si.pool.ntp.org";

const char* NTP_SERVER1 = "1.si.pool.ntp.org";

const char* NTP_SERVER2 = "2.si.pool.ntp.org";

const char* TZ_INFO2 = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";

time_t now;

struct tm info;





String txtToSend;



TaskHandle_t reportJsonTask;



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
void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
void blink(int i);
void setup();
void loop();
IRAM_ATTR void reportJson(void *pvParameters);
void waitForIp();
#line 147 "C:/Users/klemen/Dropbox/Voga/esp32_asyncws/src/Server.ino"
void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){

  if(type == WS_EVT_CONNECT){



    printf("%d ws[%s][%u] connect\n", millis(), server->url(), client->id());

    client->printf("Hello Client %u :)", client->id());

    client->ping();

  } else if(type == WS_EVT_DISCONNECT){



    printf("%d ws[%s][%u] disconnect\n", millis(), server->url(), client->id());

  } else if(type == WS_EVT_ERROR){



    printf("%d ws[%s][%u] error(%u): %s\n", millis(), server->url(), client->id(), *((uint16_t*)arg), (char*)data);

  } else if(type == WS_EVT_PONG){



    printf("%d ws[%s][%u] pong[%u]: %s\n", millis(), server->url(), client->id(), len, (len)?(char*)data:"");

  } else if(type == WS_EVT_DATA){



    AwsFrameInfo * info = (AwsFrameInfo*)arg;

    if(info->final && info->index == 0 && info->len == len){



      printf("%d ws[%s][%u] %s-message[%llu]: ", millis(), server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

      if(info->opcode == WS_TEXT){

        data[len] = 0;

        printf("%s\n", (char*)data);

      } else {

        printf("not text\n");

        for(size_t i=0; i < info->len; i++){

          printf("%02x ", data[i]);

        }

        printf("\n");

      }
# 221 "C:/Users/klemen/Dropbox/Voga/esp32_asyncws/src/Server.ino"
    } else {

      printf("multi frames\n");



      if(info->index == 0){

        if(info->num == 0)

          printf("%d ws[%s][%u] %s-message start\n", millis(), server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");

        printf("%d ws[%s][%u] frame[%u] start[%llu]\n", millis(), server->url(), client->id(), info->num, info->len);

      }



      printf("%d ws[%s][%u] frame[%u] %s[%llu - %llu]: ", millis(), server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);

      if(info->message_opcode == WS_TEXT){

        printf("%s\n", (char*)data);

      } else {

        for(size_t i=0; i < len; i++){

          printf("%02x ", data[i]);

        }

        printf("\n");

      }



      if((info->index + len) == info->len){

        printf("%d ws[%s][%u] frame[%u] end[%llu]\n", millis(), server->url(), client->id(), info->num, info->len);

        if(info->final){

          printf("%d ws[%s][%u] %s-message end\n", millis(), server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
# 279 "C:/Users/klemen/Dropbox/Voga/esp32_asyncws/src/Server.ino"
        }

      }

    }

  }

}



void blink(int i)

{

  for(int j=0; j<i; j++)

  {

    digitalWrite(LED_PIN, HIGH);

    usleep(50000);

    digitalWrite(LED_PIN, LOW);

    usleep(40000);

  }

}





void setup()

{

  Serial.begin(115200);

  Serial.print("ESP ChipSize:");

  Serial.println(ESP.getFlashChipSize());



  vTaskDelay(3 / portTICK_PERIOD_MS);

  blink(2);



  Serial.println("starting...");

  WiFiEventId_t eventID = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){

    Serial.print("WiFi lost connection. Reason: ");

    Serial.println(info.disconnected.reason);



    String msg;

    msg.concat(info.disconnected.reason);

  }, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);



  if(!ssid.equals(""))

  {

    WiFi.begin(ssid.c_str(), password.c_str());

  }

  waitForIp();



  Serial.println("Config ntp time...");

  configTzTime(TZ_INFO2, NTP_SERVER0, NTP_SERVER1, NTP_SERVER2);



  time(&now);

  localtime_r(&now, &info);



  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){

    Serial.println("/");

    Serial.println("redirecting to /index.html");

    request->redirect("/index.html");

  });



  if(!SPIFFS.begin()){

      Serial.println("SPIFFS Mount Failed");

  }

  listDir(SPIFFS, "/", 0);



  ws.onEvent(onEvent);

  server.addHandler(&ws);



  server.serveStatic("/", SPIFFS, "/").setCacheControl("max-age=30").setDefaultFile("index.html");



  server.begin();

  ArduinoOTA.begin();



  ArduinoOTA.onStart([]() {



    SPIFFS.end();



    vTaskSuspend(reportJsonTask);







    ws.enable(false);





    ws.textAll("OTA Update Started");





    ws.closeAll();

  });



  xTaskCreatePinnedToCore(

                    reportJson,

                    "reportJsonTask",

                    30000,

                    NULL,

                    tskIDLE_PRIORITY,

                    &reportJsonTask,

                    1);



  blink(5);

}



long i = 0;

void loop(){

  ArduinoOTA.handle();





  vTaskDelay(20 / portTICK_PERIOD_MS);

}





IRAM_ATTR void reportJson(void *pvParameters)

{

  if(ws.count()>0)

  {

    reportingJson = true;

    txtToSend = "";

    txtToSend.concat("{");



    txtToSend.concat("\"encoder1_value\":");

    txtToSend.concat(234234);

    txtToSend.concat(",");



    txtToSend.concat("\"encoder2_value\":");

    txtToSend.concat(234234);

    txtToSend.concat(",");



    txtToSend.concat("\"pwm1\":");

    txtToSend.concat(234);

    txtToSend.concat(",");



    txtToSend.concat("\"pwm2\":");

    txtToSend.concat(234);

    txtToSend.concat(",");



    txtToSend.concat("\"target1\":");

    txtToSend.concat(342);

    txtToSend.concat(",");



    txtToSend.concat("\"target2\":");

    txtToSend.concat(234234);

    txtToSend.concat(",");



    txtToSend.concat("\"output1\":");

    txtToSend.concat(23243);

    txtToSend.concat(",");



    txtToSend.concat("\"output2\":");

    txtToSend.concat(234234234);

    txtToSend.concat(",");



    txtToSend.concat("\"an1\":");

    txtToSend.concat(2342);

    txtToSend.concat(",");

    txtToSend.concat("\"an2\":");

    txtToSend.concat(234234);

    txtToSend.concat(",");



    txtToSend.concat("\"actual_diff\":");

    txtToSend.concat(234234);

    txtToSend.concat(",");



    txtToSend.concat("\"PID1output\":");

    txtToSend.concat("\"Pout=");

    txtToSend.concat(2342342);

    txtToSend.concat("<br>Iout=");

    txtToSend.concat(234234234);

    txtToSend.concat("<br>Dout=");

    txtToSend.concat(23424);

    txtToSend.concat("<br>Fout=");

    txtToSend.concat(234234);

    txtToSend.concat("<br>POSout=");

    txtToSend.concat(234234);

    txtToSend.concat("<br>POSoutF=");

    txtToSend.concat(2324234);

    txtToSend.concat("<br>setpoint=");

    txtToSend.concat(232342);

    txtToSend.concat("<br>actual=");

    txtToSend.concat(234234);

    txtToSend.concat("<br>error=");

    txtToSend.concat(234234);

    txtToSend.concat("<br>errorSum=");

    txtToSend.concat(234234);

    txtToSend.concat("<br>maxIOutput=");

    txtToSend.concat(23234234);

    txtToSend.concat("\",");



    txtToSend.concat("\"PID1output\":");

    txtToSend.concat("\"Pout=");

    txtToSend.concat(2342342);

    txtToSend.concat("<br>Iout=");

    txtToSend.concat(234234234);

    txtToSend.concat("<br>Dout=");

    txtToSend.concat(23424);

    txtToSend.concat("<br>Fout=");

    txtToSend.concat(234234);

    txtToSend.concat("<br>POSout=");

    txtToSend.concat(234234);

    txtToSend.concat("<br>POSoutF=");

    txtToSend.concat(2324234);

    txtToSend.concat("<br>setpoint=");

    txtToSend.concat(232342);

    txtToSend.concat("<br>actual=");

    txtToSend.concat(234234);

    txtToSend.concat("<br>error=");

    txtToSend.concat(234234);

    txtToSend.concat("<br>errorSum=");

    txtToSend.concat(234234);

    txtToSend.concat("<br>maxIOutput=");

    txtToSend.concat(23234234);

    txtToSend.concat("\",");



    txtToSend.concat("\"esp32_heap\":");

    txtToSend.concat(ESP.getFreeHeap());

    txtToSend.concat("}");



    ws.textAll(txtToSend.c_str());

  }

}





void waitForIp()

{

  while (WiFi.status() != WL_CONNECTED) {



    delay(1000);

    Serial.print("SSID: ");

    Serial.println(ssid);

    Serial.print("password: ");

    Serial.println(password);

    Serial.print("status: ");

    Serial.println(WiFi.status());



    Serial.print(".");

  }



  Serial.print("status: ");

  Serial.println(WiFi.status());



  Serial.print("WiFi local IP: ");

  Serial.println(WiFi.localIP());

}