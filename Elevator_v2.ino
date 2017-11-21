#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>

int readStringFromSerial(char *buffer, int max_len );
void serialFlush();

File uploadFile;
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void outbound_transfer(String filename);

// SKETCH BEGIN
AsyncWebServer server(80);
const char* http_username = "admin";
const char* http_password = "admin";
const char* ssid = "Elevator_v2";
const char* ssid_password = "";
const char * hostName = "esp-async";

bool shouldReboot = false; //flag to use from web update to reboot the ESP
unsigned int transferCommand = 0; //1 = inbound trasnfer; 2 = outbound transfer
String cfg_name = "B8_KRP.txt";

void setup() {
  Serial.begin(9600);
  Serial.setDebugOutput(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, ssid_password, 1, 0);
  //ssid, password, channel, hidden
  //  WiFi.hostname(hostName);
  //  WiFi.mode(m): set mode to WIFI_AP, WIFI_STA, WIFI_AP_STA
  //  WiFi.mode(WIFI_STA);
  //  WiFi.softAP(hostName);
  //  WiFi.begin(ssid, ssid_password);
  //  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
  //    Serial.printf("STA: Failed!\n");
  //    WiFi.disconnect(false);
  //    delay(1000);
  ////    WiFi.begin(ssid, ssid_password);
  //  }
  MDNS.addService("http", "tcp", 80);

  SPIFFS.begin();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (request->hasParam("action") && request->hasParam("chip1")) {
      String getaction = request->getParam("action")->value().c_str();
      String getchip1 = request->getParam("chip1")->value().c_str();
      serialFlush();
      Serial.println(getaction + getchip1);
      request->redirect("/");
      //request->send(SPIFFS, "/index.html");
    }
    else {
      //      Serial.println("server on /");
      //      if (!request->authenticate(http_username, http_password)) {
      //        Serial.println(request);
      //        return request->requestAuthentication();
      //
      request->send(SPIFFS, "/index.html");
      //      }
    }
  });

  server.on("/admin.html", HTTP_POST, [](AsyncWebServerRequest * request) {
    //    Serial.println("/admin.html post");
    //    handleUpload();
    //    request->send(SPIFFS, "/admin.html");
    //  });
    //    AsyncWebParameter* p = request->getParam("file");
    //    if (p->isFile()) { //p->isPost() is also true
    //    if (request->getParam("file", true)->isFile()) {
    //      Serial.print("File uploaded");
    //      //      Serial.printf("FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
    //    }
    //    else {
    //      Serial.print("No file uploaded");
    //    }
    //    int args = request->args();
    //    for (int i = 0; i < args; i++) {
    //      Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(), request->arg(i).c_str());
    //    }
    String rtcode;
    if (request->hasParam("file", true, true)) rtcode = "OK";
    else rtcode = "FAIL";
    
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", rtcode);
    response->addHeader("Connection", "close");
    request->send(response);
    //request->send(SPIFFS, "/admin.html?status=uploaded");
  }, handleUpload);

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
  //  // Admin dialog

  server.addHandler(new SPIFFSEditor(http_username, http_password));
  // HTTP basic authentication
  //  server.on("/admin.html", HTTP_GET, [](AsyncWebServerRequest * request) {
  //    if (!request->authenticate(http_username, http_password)) {
  //      return request->requestAuthentication();
  //      request->send(200, "text/plain", "Login Success!");
  //    }
  //  });

  server.serveStatic("/admin.html", SPIFFS, "/admin.html").setAuthentication(http_username, http_password);

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

void outbound_transfer(String filename) {
  //filename = "/style.css";
  filename = "/" + cfg_name;
  File f = SPIFFS.open(filename, "r");
  size_t fsize = f.size();
  const byte numChars = 512;
  String receivedConfirmation;
  byte rc;
  bool moredata = 1; //there are bytes in the file that need to be send
  //prepare receiver
  Serial.print("rcv");
  delay(50);
  while (moredata == 1) {
    //read file f and send byte by byte up to numChars bytes (512)
    for (int i = 0; moredata == 1 && i < 512; i++) {
      rc = f.read();
      Serial.write(rc);
      if (f.available() > 0) {
        moredata = 1;
      }
      else moredata = 0;
    }
    //Wait until the remote MCU is ready and send a response
    while (!Serial.available()) {
      yield();
      delay(1000);
      //      Serial.println("Waiting for 'OK'");
    }
    //read the response from the remote MCU
    receivedConfirmation = "";
    for (int k = 0; receivedConfirmation != "OK"; ) {
      while (Serial.available() > 0) {
        char t = Serial.read();
        receivedConfirmation += t;
      }
    }
  }
  f.close();
  transferCommand = 0;
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    //Serial.printf("UploadStart: %s\n", filename.c_str());
    if (!filename.startsWith("/"))
      filename = "/" + cfg_name;
    if (SPIFFS.exists(filename)) {
      SPIFFS.remove(filename);
    }
    //insted of uploadFile
    request->_tempFile = SPIFFS.open(filename, "w");
    //filename = String();
  }
  for (size_t i = 0; i < len; i++) {
    request->_tempFile.write(data[i]);
  }
  if (final) {
    request->_tempFile.close();
    //    uploadFile = SPIFFS.open(filename, "r");
    //    if(!uploadFile) Serial.printf("Greshka: %s, %u B\n", filename.c_str(), index + len);
    //    Serial.printf("UploadEnd: %s, %u B\n", filename.c_str(), index + len);
    //outbound_transfer(filename);
    transferCommand = 2; //change the command so main loop can trigger the outbound transfer
  }
}
void loop() {

  yield();

  if (transferCommand == 2) outbound_transfer(cfg_name);

  if (shouldReboot) {
    //Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
  }
}

