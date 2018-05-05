#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include "HtmlPage.h"

MDNSResponder mdns;
ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Состояние реле
// 0 - Light;
// 1 - Pump;
// 2 - Compressor;
// 3 - Oilspill;
bool releStatus[4];

bool lightKeyStatus = false;

const int lightPin = 16;
const int pumpPin = 5;
const int oilpillPin = 4;
const int compressorPin = 14;

const int keyPin = 17;

// Название Wifi и пароль к нему
static const char ssid[]     = "TP-LINK_FC7E74";//"TP-LINK_FC7E74"
static const char password[] = "26911908"; //"26911908"
bool isWifiConnected = true;

void sendChangesReleStatusToClient(int num, int id, byte s) {
  id = id;
  char myConcatenation[10];
  if (s) {
    char mes[] = "on";
    sprintf(myConcatenation, "%i %s", id, mes);
  } else {
    char mes[] = "off";
    sprintf(myConcatenation, "%i %s", id, mes);
  }
  webSocket.sendTXT(num, myConcatenation, strlen(myConcatenation));
}

void sendChangesReleStatusToClients(int id, byte s) {
  id = id;
  char myConcatenation[10];
  if (s) {
    char mes[] = "on";
    sprintf(myConcatenation, "%i %s", id, mes);

  } else {
    char mes[] = "off";
    sprintf(myConcatenation, "%i %s", id, mes);
  }
  webSocket.broadcastTXT(myConcatenation, strlen(myConcatenation));
}

void handleRoot()
{
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleNotFound()
{
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

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        // Отправка первичных данных только что подключенному клиенту
        for (int i = 1; i < 4; i++) {
          sendChangesReleStatusToClient(num, i, releStatus[i]);
        }
      }
      break;
    case WStype_TEXT:
      // Обработка запроса от клиента
      Serial.printf("[%u] get Text: %s\r\n", num, payload);

      toggleRelePin(atoi((const char *)payload));

      //      // send data to all connected clients
      //      webSocket.broadcastTXT(payload, length);
      break;
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\r\n", num, length);
      hexdump(payload, length);

      // echo data back to browser
      webSocket.sendBIN(num, payload, length);
      break;
    default:
      Serial.printf("Invalid WStype [%d]\r\n", type);
      break;
  }
}

void waitForConnect()
{
  bool isLoginGet = false;
  String ssid1;
  long milli = millis();
  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
    if (millis() - milli > 20000) {
      isWifiConnected = false;
      break;
    }

    while (Serial.available()) {
      Serial.println("String get");
      String a = Serial.readString();
      if (!isLoginGet) {
        ssid1 = a;
        isLoginGet = true;
      } else {
        char ssidChar[40];
        char passwordChar[40];
        ssid1.toCharArray(ssidChar, 40);
        a.toCharArray(passwordChar, 40);

        WiFiMulti.addAP(ssidChar, passwordChar);
      }
    }
  }
}

void setupSTA() {
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (mdns.begin("espWebSock", WiFi.localIP())) {
    Serial.println("MDNS responder started");
    mdns.addService("http", "tcp", 80);
    mdns.addService("ws", "tcp", 81);
  }
  else {
    Serial.println("MDNS.begin failed");
  }
  Serial.print("Connect to http://");
  Serial.println(WiFi.localIP());
}

void setup_AP_STA() {
  WiFi.mode(WIFI_AP_STA);
  Serial.println("Setting soft-AP ... ");
  boolean result = WiFi.softAP("ESP_Alekseyld", "123456789");
  if (result == true)
  {
    Serial.println("Ready");
  }
  else
  {
    Serial.println("Failed!");
  }
}

void setup()
{
  WiFi.mode(WIFI_STA);
  
  setupPins();

  Serial.begin(115200);
  Serial.setDebugOutput(true);

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\r\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFiMulti.addAP(ssid, password);

  waitForConnect();

  Serial.println();

  if (isWifiConnected) {
    Serial.println("isWifiConnected = true");
    setupSTA();
  } else {
    Serial.println("isWifiConnected = false");
    setup_AP_STA();
  }

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void writeRelePins(int id, bool toogle) {
  int pin = 0;
  
  switch (id) {
    case 0:
      pin = lightPin;
      break;
    case 1:
      pin = pumpPin;
      break;
    case 2:
      pin = oilpillPin;
      break;
    case 3:
      pin = compressorPin;
      break;
  }

  digitalWrite(pin, toogle ? HIGH : LOW);
}

void toggleRelePin(int id) {
//  if (id == 0 && lightKeyStatus){
//     return;
//  }

  bool toogle = !releStatus[id];
  
  releStatus[id] = toogle;
  writeRelePins(id, toogle);
  sendChangesReleStatusToClients(id, toogle);
}

void processLightKey() {
  lightKeyStatus = analogRead(keyPin) > 500 ? !lightKeyStatus : lightKeyStatus;

  releStatus[0] = lightKeyStatus;
  digitalWrite(lightPin, lightKeyStatus ? HIGH : LOW);
  sendChangesReleStatusToClients(0, lightKeyStatus ? HIGH : LOW);
}

// Опрос всех подключенных датчиков и присвоение им значений в статусе
void monitorPins() {
  processLightKey();
}

void setupPins()
{
  pinMode(lightPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(oilpillPin, INPUT);
  pinMode(compressorPin, OUTPUT);
  pinMode(keyPin, INPUT);

  processLightKey();
}

long lastmillis = 0;

void loop() {
  webSocket.loop();
  server.handleClient();

  if (millis() - lastmillis > 300) {
    lastmillis = millis();
    monitorPins();
  }
}
