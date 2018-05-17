
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include <ArduinoJson.h>

int readStringFromSerial(char *buffer, int max_len );
int readByteFromSerial(byte *buffer, int max_len );
void serialFlush();

File uploadFile;
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void outbound_transfer(String filename);
void inbound_transfer();
String check_filename();

// SKETCH BEGIN
AsyncWebServer server(80);
const char* http_username = "admin";
const char* http_password = "admin";
const char* ssid = "Elevator_v2";
const char* ssid_password = "";
const char * hostName = "esp-async";

bool shouldReboot = false; //flag to use from web update to reboot the ESP
unsigned int transferCommand = 0; //1 = inbound trasnfer; 2 = outbound transfer
char flagCommand = ' '; // ' ' = no command; 's' = set config;
String cfg_name = "B8_KRP.txt";
String backup_bin;

void setup() {
  Serial.begin(9600);
  Serial.setDebugOutput(false);
  WiFi.mode(WIFI_AP);
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

  backup_bin = check_filename();

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
      request->send(SPIFFS, "/index.html");
    }
  });

  server.on("/ajax", HTTP_POST, [](AsyncWebServerRequest * request) {
    String action;
    String rtcode;
    if (request->hasParam("action", true)) {
      action = request->getParam("action", true)->value().c_str();
      if (action == "snd_file") {
        rtcode = "PREP";
        transferCommand = 1;
      }
      else if (action == "chk_config") {
        if (backup_bin != "")  rtcode = "CONFIG_OK";
        else rtcode = "NO_CONFIG";
      }
      else if (action == "set_config") {
        backup_bin = request->getParam("config_name", true)->value().c_str();
        flagCommand = 's';
        rtcode = "SET_CONFIG";
      }
    }
    else rtcode = "FAIL";
    //application/json
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", rtcode);
    response->addHeader("Connection", "close");
    request->send(response);
  });

  server.on("/admin.html", HTTP_POST, [](AsyncWebServerRequest * request) {
    String rtcode;
    String action;

    //upload file
    if (request->hasParam("file", true, true)) {
      rtcode = "OK";
    }

    //do nothing
    else {
      rtcode = "FAIL";
    }

    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", rtcode);
    response->addHeader("Connection", "close");
    request->send(response);
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

  server.addHandler(new SPIFFSEditor(http_username, http_password));

  server.serveStatic("/admin.html", SPIFFS, "/admin.html").setAuthentication(http_username, http_password);

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  server.begin();
}

//check if config.json exists and filename for backup is valid
String check_filename() {
  if (SPIFFS.exists("/config.json")) {
    File f = SPIFFS.open("/config.json", "r");
    int fileSize = f.size();
    if (fileSize > 255) {
      f.close();
      SPIFFS.remove("/config.json");
      File f = SPIFFS.open("/config.json", "w+");
      f.close();
    }
  }
  else {
    File f = SPIFFS.open("/config.json", "w+");
    //    StaticJsonBuffer<200> jsonBuffer;
    //    JsonObject& json = jsonBuffer.createObject();
    //    json["filename"] = "backup";
    //    json.printTo(f);
    f.close();
  }

  File f = SPIFFS.open("/config.json", "r");
  int fileSize = f.size();

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[fileSize]);

  f.readBytes(buf.get(), fileSize);
  f.close();

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(buf.get());
  if (!root.success()) {
    Serial.println("parseObject() failed");
  }
  else {
    const char* filename = root["filename"];
    return filename;
  }
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

int readByteFromSerial(byte *buffer, int max_len )
{
  int pos = 0;
//  buffer[pos] = '\0';
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
      byte readch = Serial.read();
//      if (readch == '\n') {
//        //        Serial.println("serial received \n");
//        break;
//      } else if (  pos < max_len - 1) {
//        //        Serial.println("serial is filling up buffer");
        buffer[pos++] = readch;
//      }
    }
  }
  
  //  Serial.println("buffer is filled; return");
//  buffer[pos] = '\0';
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
    //    //Wait until the remote MCU is ready and send a response
    //    while (!Serial.available()) {
    //      yield();
    //      delay(1000);
    //      //      Serial.println("Waiting for 'OK'");
    //    }
    //read the response from the remote MCU
    receivedConfirmation = "";
    for (int k = 0; receivedConfirmation != "OK"; ) {
      yield();
      delay(1000);
      while (Serial.available() > 0) {
        char t = Serial.read();
        receivedConfirmation += t;
      }
    }
  }
  f.close();
  transferCommand = 0;
}

void inbound_transfer() {
  String filename = "/backup.bin";
  if (SPIFFS.exists(filename)) {
      SPIFFS.remove(filename);
  }
  File f = SPIFFS.open(filename, "w");
  size_t fsize = f.size();
  const byte numChars = 512;
  String receivedConfirmation;
  byte rc;
  bool moredata = 1; //there are bytes in the file that need to be received
  //prepare sender
  Serial.print("snd");
  delay(10);
  while (moredata == 1) {
    // READ ANSWER FROM REMOTE DEVICE

    byte buff[1024];

    int len =  readByteFromSerial(buff, 1024);
     
    //serialFlush(); //read everithing from serial

    if (len > 0) {
      len = 0;
      f.write(buff, 1024);
    }
   if (Serial.available() > 0) {
        moredata = 1;        
        yield();
    }
    else moredata = 0;
    }
//    //read Serial and send byte by byte up to file f numChars bytes (512)
//   byte inData[512];
//    for (int i = 0; moredata == 1 && i < 512; i++) {
//      inData[i] = Serial.read();
//      //int k = Serial.read();
//      //f.write(k);
//      //f.write(rc);
//      if (Serial.available() > 0) {
//        moredata = 1;        
//        yield();
//      }
//      else moredata = 0;
//    }
//    for (int i = 0; i < 512; i++) {
//      f.write(inData[i]);
//      yield();
//    }
//    //    //Wait until the remote MCU is ready and send a response
//    //    while (!Serial.available()) {
//    //      yield();
//    //      delay(1000);
//    //      //      Serial.println("Waiting for 'OK'");
//    //    }
//    //send the response to the remote MCU
//    //    receivedConfirmation = "";
//    //    for (int k = 0; receivedConfirmation != "OK"; ) {
//    //      yield();
//    //      delay(200);
//    //      while (Serial.available() > 0) {
//    //        char t = Serial.read();
//    //        receivedConfirmation += t;
//    //      }
//    //    }
//  }
  f.close();
  transferCommand = 0;
}

void handleUpload(AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
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
void set_config() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["filename"] = backup_bin;

  File f = SPIFFS.open("/config.json", "w");
  //  if (!f) {
  //    Serial.println("Failed to open config file for writing");
  ////    return false;
  //  }

  json.printTo(f);
  flagCommand = ' ';
  //  return true;
}

void loop() {

  yield();

  if (transferCommand == 2) outbound_transfer(cfg_name);
  if (transferCommand == 1) inbound_transfer();
  if (flagCommand == 's') set_config();

  if (shouldReboot) {
    //Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
  }
}

