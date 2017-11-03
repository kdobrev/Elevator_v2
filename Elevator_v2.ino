#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>

int readStringFromSerial(char *buffer, int max_len );
void serialFlush();

// SKETCH BEGIN
AsyncWebServer server(80);
const char* http_username = "admin";
const char* http_password = "";
const char* ssid = "Elevator";
const char* ssid_password = "";
//flag to use from web update to reboot the ESP
bool shouldReboot = false;

void setup() {
  Serial.begin(9600);
  Serial.setDebugOutput(false);
  WiFi.mode(WIFI_AP_STA);

  WiFi.softAP(ssid, ssid_password, 1, 0);
  //ssid, password, channel, hidden

  MDNS.addService("http", "tcp", 80);

  SPIFFS.begin();
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (request->hasParam("action") && request->hasParam("chip1")) {
      String getaction = request->getParam("action")->value().c_str();
      String getchip1 = request->getParam("chip1")->value().c_str();
      serialFlush();
      Serial.println(getaction + getchip1);
      request->redirect("/");
    }
    //else {
      //      if (!request->authenticate(http_username, http_password))
      //        return request->requestAuthentication();
      //request->send(SPIFFS, "/index.html");
    //}
  });

  server.on("/", HTTP_POST, [](AsyncWebServerRequest * request) {

    String command = "";
    String txcommand = "";
    command = request->getParam("action", true)->value();

    //      checkg <check apartment number>
    if (command == "checkg")
    {
      if (request->getParam("ap", true)->value() != "")
      {
        txcommand = command + request->getParam("ap", true)->value();
      }
    }
    //        save
    else if (command == "save") {
      txcommand = command + request->getParam("ap", true)->value();

      for (int j = 0, params = request->params(); j < params; j++) {
        AsyncWebParameter* p = request->getParam(j);
        if (p->name().startsWith("chip"))
        {
          String v = p->value();
          if (v.length() > 0) {
            txcommand += "*" + v;
          }
        }
      }
    }
    //        wait
    else if (command == "wait") {
      txcommand = command + request->getParam("ap", true)->value();
      for (int j = 0, params = request->params(); j < params; j++) {
        AsyncWebParameter* p = request->getParam(j);
        if (p->name().startsWith("wait"))
        {
          String v = p->value();
          if (v.length() > 0) {
            txcommand += "*" + v;
          }
        }
      }
    }
    //        erasen
    else if (command == "erasen") {
      txcommand = command + request->getParam("chip1", true)->value();
    }
    //        eraseg
    else if (command == "eraseg") {
      txcommand = command + request->getParam("ap", true)->value();
    }
    //        time
    else if (command == "time") {
      txcommand = command + request->getParam("time", true)->value();
    }
    //        repid
    else if (command == "repid") {
      txcommand = command + request->getParam("repid", true)->value();
    }
    //        glock
    else if (command == "glock") {
      txcommand = command + request->getParam("ap", true)->value();
    }
    //        gunlock
    else if (command == "gunlock") {
      txcommand = command + request->getParam("ap", true)->value();
    }

    serialFlush();
    Serial.println(txcommand);

    // READ ANSWER FROM REMOTE DEVICE

    char buff[1024];
    String serial_response = "";
    int len =  readStringFromSerial(buff, 1024);

    serialFlush(); //read everithing from serial

    if (len > 0) {
      serial_response = buff;
      len = 0;
      request->redirect("/?r=" + serial_response);
      serial_response = "";
    }
    else request->redirect("/");
  });

  // Simple Firmware Update Form
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/html", "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>");
  });
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest * request) {
    shouldReboot = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : "FAIL");
    response->addHeader("Connection", "close");
    request->send(response);
  }, [](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
      Serial.printf("Update Start: %s\n", filename.c_str());
      Update.runAsync(true);
      if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
        Update.printError(Serial);
      }
    }
    if (!Update.hasError()) {
      if (Update.write(data, len) != len) {
        Update.printError(Serial);
      }
    }
    if (final) {
      if (Update.end(true)) {
        Serial.printf("Update Success: %uB\n", index + len);
      } else {
        Update.printError(Serial);
      }
    }
  });

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  server.begin();
}

int readStringFromSerial(char *buffer, int max_len )
{
  int pos = 0;
  buffer[pos] = '\0';
  //  Serial.println("enter in readstring and define vars");
  unsigned long int init_time = millis();
  unsigned long int wait_time = init_time + 1000;
  while (!Serial.available())
  {
    //    Serial.println("enter waiting for serial");
    if (millis() >= wait_time) {
      // no responce - just leave
      //      Serial.println("time is up");
      return pos;
    }
  }
  while (pos < max_len - 1) {

    if (Serial.available() > 0) {
      //      Serial.println("serial was available");
      int readch = Serial.read();
      if (readch == '\n') {
        //        Serial.println("serial received \n");
        break;
      } else if (  pos < max_len - 1) {
        //        Serial.println("serial is filling up buffer");
        buffer[pos++] = readch;
      }
    }
  }
  //  Serial.println("buffer is filled; return");
  buffer[pos] = '\0';
  return pos;
}
void serialFlush() {
  while (Serial.available() > 0) {
    char t = Serial.read();
  }
}
void loop() {
  //  String str;
  //  str = Serial.readString();
  //
  //  Serial.println(str);

  if (shouldReboot) {
    //Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
  }
}

