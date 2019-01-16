/*
   Split-Flap display row controller with web interface
   (C) 2017 Julian Metzler
*/

/*
   UPLOAD SETTINGS

   Board: Generic ESP8266 Module
   Flash Mode: DIO
   Flash Size: 1M
   SPIFFS Size: 256K
   Debug port: Disabled
   Debug Level: None
   Reset Method: ck
   Flash Freq: 40 MHz
   CPU Freq: 80 MHz
   Upload Speed: 115200
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>

/*
   TYPEDEFS
*/

//bla

/*
   CONSTANTS
*/

#define NUM_MODULES 24

/*
   GLOBAL VARIABLES
*/

const char* ssid = "SplitFlapTest";
const char* password = "splitflap";

char TEXT[NUM_MODULES + 1] = {0};
unsigned int ERRORS[NUM_MODULES] = {0};

unsigned long transformerOnTime = 0;

ESP8266WebServer server(80);

/*
   CONFIGURATION SAVE & RECALL (EEPROM)
*/

void EEPROMWriteByte(int address, byte value) {
  EEPROM.write(address, value);
}

void EEPROMWriteInt(int address, unsigned int value) {
  EEPROM.write(address, value);
  EEPROM.write(address + 1, value >> 8);
}

void EEPROMWriteLong(int address, unsigned long value) {
  EEPROM.write(address, value);
  EEPROM.write(address + 1, value >> 8);
  EEPROM.write(address + 2, value >> 16);
  EEPROM.write(address + 3, value >> 24);
}

byte EEPROMReadByte(int address) {
  return EEPROM.read(address);
}

unsigned int EEPROMReadInt(int address) {
  return (unsigned int)EEPROM.read(address) | (unsigned int)EEPROM.read(address + 1) << 8;
}

unsigned long EEPROMReadLong(int address) {
  return (unsigned long)EEPROM.read(address) | (unsigned long)EEPROM.read(address + 1) << 8 | (unsigned long)EEPROM.read(address + 2) << 16 | (unsigned long)EEPROM.read(address + 3) << 24;
}

void saveConfiguration() {
  for (int i = 0; i < NUM_MODULES; i++) {
    EEPROMWriteInt(i * 2, ERRORS[i]);
  }
  EEPROM.commit();
}

void loadConfiguration() {
  for (int i = 0; i < NUM_MODULES; i++) {
    ERRORS[i] = EEPROMReadInt(i * 2);
  }
}

/*
   HELPER FUNCTIONS
*/

void rs485BeginTransmission() {
  digitalWrite(0, HIGH);
  delay(10);
}

void rs485PutByte(unsigned char byte) {
  Serial.write(byte);
}

void rs485EndTransmission() {
  delay(100);
  digitalWrite(0, LOW);
}

unsigned char rs485GetByte() {
  digitalWrite(0, LOW);
  while (!Serial.available());
  return Serial.read();
}

unsigned char letterToCardPosition(unsigned char letter) {
  if (letter >= 48 && letter <= 57) {
    // 0 to 9
    return letter - 47;
  } else if (letter >= 65 && letter <= 90) {
    // A to Z
    return letter - 54;
  } else if (letter >= 97 && letter <= 122) {
    // a to z, mapped to A to Z
    return letter - 86;
  } else {
    switch (letter) {
      case '-': return 40;
      case '.': return 41;
      case '(': return 42;
      case ')': return 43;
      case '!': return 44;
      case ':': return 45;
      case '/': return 46;
      case '"': return 47;
      case ',': return 48;
      case '=': return 49;
      default:  return 0;
    }
  }
}

void setHome() {
  rs485BeginTransmission();
  rs485PutByte(0xff);
  rs485PutByte(0xff);
  rs485PutByte('h');
  rs485EndTransmission();
  //enableTransformer();
}

void setString(char* string) {
  strcpy(TEXT, string);
  rs485BeginTransmission();
  unsigned char curChar = 0xff;
  int curAddr = 0;
  int numChars = 0;
  int len = strlen(TEXT);
  unsigned char cardPos = 0;
  while (numChars < NUM_MODULES) {
    if (numChars < len) {
      curChar = *string;
      if (curChar != 0) {
        cardPos = letterToCardPosition(curChar);
      } else {
        cardPos = 0;
      }
    } else {
      cardPos = 0;
    }
    rs485PutByte(0xff);
    rs485PutByte(curAddr + 1);
    rs485PutByte('c');
    rs485PutByte(cardPos);
    string++;
    curAddr++;
    numChars++;
  }
  rs485EndTransmission();
  //enableTransformer();
}

void setString(const char* string) {
  setString((char*)string);
}

void setModule(int address, int cardPos) {
  rs485BeginTransmission();
  rs485PutByte(0xff);
  rs485PutByte(address);
  rs485PutByte('c');
  rs485PutByte(cardPos);
  rs485EndTransmission();
}

void setModuleHome(int address) {
  rs485BeginTransmission();
  rs485PutByte(0xff);
  rs485PutByte(address);
  rs485PutByte('h');
  rs485EndTransmission();
}

void enableTransformer() {
  digitalWrite(2, HIGH);
  transformerOnTime = millis();
}

void disableTransformer() {
  digitalWrite(2, LOW);
}

/*
   WEB SERVER
*/

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void handleRoot() {
  String page;
  page += "<html>";
  page += "<head>";
  page += "<link rel='shortcut icon' href='/favicon.ico'>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  page += "<link rel='stylesheet' href='/main.css'>";
  page += "<script type='text/javascript' src='/main.js'></script>";
  page += "<title>Fallblatt-Controller</title>";
  page += "</head>";
  page += "<body>";
  page += "<h1>Fallblatt-Controller</h1>";

  page += "<h2>Text eingeben</h2>";
  page += "<form action='/set-text' method='POST' style='display:inline-block;'>";
  page += "<input type='text' name='text' id='text' onkeyup='countLetters();' value='";
  page += TEXT;
  page += "'/>";
  page += "<div id='charcount' style='display:inline-block; margin-left:0.5em; margin-right:0.5em; text-align:right; width:1em;'>0</div>";
  page += "<input type='submit' value='Senden'/>";
  page += "</form>";

  page += "<form action='/set-home' method='POST' style='display:inline-block;'>";
  page += "<input type='submit' value='Null'/>";
  page += "</form>";

  char i_str[3];
  char err_str[6];
  page += "<h2>Fehler-Statistik</h2>";
  page += "<table>";
  page += "<tr>";
  for (int i = 0; i < NUM_MODULES; i++) {
    page += "<td>";
    page += "<div style='display:inline-block; text-align:center; width:100%;'>";
    page += TEXT[i];
    page += "</div>";
    page += "</td>";
  }
  page += "</tr>";
  page += "<tr>";
  for (int i = 0; i < NUM_MODULES; i++) {
    sprintf(i_str, "%i", i);
    page += "<td>";
    page += "<form action='/report-error' method='POST' style='margin:0px; text-align:center;'>";
    page += "<input type='hidden' name='pos' value='";
    page += i_str;
    page += "' />";
    page += "<input type='submit' value='F' />";
    page += "</form>";
    page += "</td>";
  }
  page += "</tr>";
  page += "<tr>";
  for (int i = 0; i < NUM_MODULES; i++) {
    sprintf(err_str, "%i", ERRORS[i]);
    page += "<td>";
    page += "<div style='display:inline-block; text-align:center; width:100%;'>";
    page += err_str;
    page += "</div>";
    page += "</td>";
  }
  page += "</tr>";
  page += "</table>";

  page += "</body>";
  page += "</html>";
  server.send(200, "text/html", page);
}

void handle_set_text() {
  setString(server.arg("text").c_str());
  server.sendHeader("Location", "/", true);
  server.send(303, "text/plain", "");
}

void handle_set_home() {
  setHome();
  server.sendHeader("Location", "/", true);
  server.send(303, "text/plain", "");
}

void handle_report_error() {
  int pos = atoi(server.arg("pos").c_str());
  unsigned char letter = TEXT[pos];
  int cardPos = letterToCardPosition(letter);
  setModuleHome(pos + 1);
  setModule(pos + 1, cardPos);
  ERRORS[pos]++;
  saveConfiguration();
  server.sendHeader("Location", "/", true);
  server.send(303, "text/plain", "");
}

/*
   MAIN PROGRAM
*/

void setup() {
  ArduinoOTA.setHostname("Fallblatt-Controller");
  ArduinoOTA.begin();

  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);

  digitalWrite(0, LOW);
  digitalWrite(2, HIGH);

  EEPROM.begin(512);
  SPIFFS.begin();

  loadConfiguration();

  WiFi.mode(WIFI_STA);
  WiFi.hostname("Fallblatt-Controller");
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(5000);
  }

  server.onNotFound(handleNotFound);
  server.on("/", handleRoot);
  server.on("/set-text", handle_set_text);
  server.on("/set-home", handle_set_home);
  server.on("/report-error", handle_report_error);
  server.serveStatic("/main.css", SPIFFS, "/main.css");
  server.serveStatic("/main.js", SPIFFS, "/main.js");
  server.serveStatic("/favicon.ico", SPIFFS, "/favicon.ico");
  server.begin();

  Serial.begin(9600);

  //setHome();
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  /*if ((millis() - transformerOnTime) > 5000) {
    disableTransformer();
    }*/
}
