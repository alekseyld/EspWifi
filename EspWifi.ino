#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include "HtmlPage.h"

// Название Wifi и пароль к нему

static const char ssid[]     = "TP-LINK_FC7E74";//"TP-LINK_FC7E74"
static const char password[] = "26911908"; //"26911908"
MDNSResponder mdns;

void toggleRelePin(int);

ESP8266WiFiMulti WiFiMulti;

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Состояние реле
// 0 - PumpStatus;
// 1 - SnakefunStatus;
// 2 - VapofanStatus;
// 3 - MeshalkaStatus;
// 4 - LightStatus;
byte ReleStatus = 0;

// Состояния тумблеров
// 0 - PumpKeyStatus;
// 1 - SnakefunKeyStatus;
// 2 - VapofanKeyStatus;
// 3 - MeshalkaKeyStatus;
// 4 - LightStatus;
byte KeyStatus;

// Состояние датчиков температуры
int Temp1Status;
// Состояние датчиков уровня
bool HeightStatus;

//Pins
int latchPin = 15; // pin D8 on NodeMCU boards
int clockPin = 14; // pin D5 on NodeMCU boards
int dataPin = 13; // pin D7 on NodeMCU boards

const int S0 = 16;
const int S1 = 5;
const int S2 = 4;
const int S3 = 12;
const int SIG = 17;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        // Отправка первичных данных только что подключенному клиенту
        for (int i = 1; i < 6; i++){
          sendChangesReleStatusToClient(num, i, bitRead(ReleStatus, i));
        }
      }
      break;
    case WStype_TEXT:
      // Обработка запроса от клиента
      Serial.printf("[%u] get Text: %s\r\n", num, payload);

      toggleRelePin(atoi((const char *)payload) + 1);
      
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
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

//void registerWrite(int whichPin, int whichState) {
//// инициализируем и обнуляем байт
//  byte bitsToSend = 0;
//
//  //Отключаем вывод на регистре
//  digitalWrite(latchPin, LOW);
//
//  // устанавливаем HIGH в соответствующем бите
//  bitWrite(bitsToSend, whichPin, whichState);
//
//  // проталкиваем байт в регистр
//  shiftOut(dataPin, clockPin, MSBFIRST, bitsToSend);
//
//    // "защелкиваем" регистр, чтобы байт появился на его выходах
//  digitalWrite(latchPin, HIGH);
//}

int readMux(int channel){
  int controlPin[] = {S0, S1, S2, S3};

  int muxChannel[16][4]={
    {0,0,0,0}, //channel 0
    {1,0,0,0}, //channel 1
    {0,1,0,0}, //channel 2
    {1,1,0,0}, //channel 3
    {0,0,1,0}, //channel 4
    {1,0,1,0}, //channel 5
    {0,1,1,0}, //channel 6
    {1,1,1,0}, //channel 7
    {0,0,0,1}, //channel 8
    {1,0,0,1}, //channel 9
    {0,1,0,1}, //channel 10
    {1,1,0,1}, //channel 11
    {0,0,1,1}, //channel 12
    {1,0,1,1}, //channel 13
    {0,1,1,1}, //channel 14
    {1,1,1,1}  //channel 15
  };

  //loop through the 4 sig
  for(int i = 0; i < 4; i ++){
    digitalWrite(controlPin[i], muxChannel[channel][i]);
  }

  //read the value at the SIG pin
  int val = analogRead(SIG);

  //return the value
  return val;
}

void writeRelePins() {
//  byte bitsToSend = 0;
//  EEPROM.write(addr, val);
//  
//  for (int i = 0; i < 8; i++) {
//    if (ReleStatus[i]) {
//      bitWrite(bitsToSend, i, HIGH);
//    } else {
//      bitWrite(bitsToSend, i, LOW);
//    }
//  }
  //Отключаем вывод на регистре
  digitalWrite(latchPin, LOW);

  // проталкиваем байт в регистр
  shiftOut(dataPin, clockPin, MSBFIRST, ReleStatus);

    // "защелкиваем" регистр, чтобы байт появился на его выходах
  digitalWrite(latchPin, HIGH);
}

void sendChangesReleStatusToClient(int num, int id, byte s) {
  id = id - 1;
  char myConcatenation[10];
  if (s) {
    char mes[] = "on"; 
    sprintf(myConcatenation,"%i %s",id, mes);
  } else {
    char mes[] = "off"; 
    sprintf(myConcatenation,"%i %s",id, mes);
  }
  webSocket.sendTXT(num, myConcatenation, strlen(myConcatenation));
}

void sendChangesReleStatusToClients(int id, byte s) {
  id = id - 1;
  char myConcatenation[10];
  if (s) {
    char mes[] = "off"; 
    sprintf(myConcatenation,"%i %s",id, mes);
    
  } else {
    char mes[] = "on"; 
    sprintf(myConcatenation,"%i %s",id, mes);
  }
  webSocket.broadcastTXT(myConcatenation, strlen(myConcatenation));
}

void toggleRelePin(int id) {
  //Проверять состояние тумблера
//  if (bitRead(KeyStatus, id)) {
//    return;
//  }

  byte b = bitRead(ReleStatus, id);
  if (b == 0) {
    bitWrite(ReleStatus, id, HIGH);
  } else {
    bitWrite(ReleStatus, id, LOW);
  }

  //EEPROM.write(0, b);
  //EEPROM.commit();

  writeRelePins();
  sendChangesReleStatusToClients(id, b);
}
//  bool b = !ReleStatus[id];
//  Serial.printf("toggleRelePin: %d to %d\r\n", id, b);
//  
//  if (KeyStatus[id] && !b) {
//    return;
//  }
//
//  ReleStatus[id] = b;
//  if (b) {
//    char myConcatenation[10];
//    char mes[] = "on"; 
//    sprintf(myConcatenation,"%i %s",id, mes);
//    webSocket.broadcastTXT(myConcatenation, strlen(myConcatenation));
//  } else {
//    char myConcatenation[10];
//    char mes[] = "off"; 
//    sprintf(myConcatenation,"%i %s",id, mes);
//    webSocket.broadcastTXT(myConcatenation, strlen(myConcatenation));
//  }
//  writeRelePins();

void sendUpdatesToClients() {
  char tempMes[10];
  sprintf(tempMes,"0 %d", Temp1Status);
  webSocket.broadcastTXT(tempMes, strlen(tempMes));

  //Отправка показаний датчика уровня
  char heightMes[20];
  if (HeightStatus) {
    char mes[] = "Достигнут"; 
    sprintf(heightMes,"0 %s", mes);
    
  } else {
    char mes[] = "Не_достигнут"; 
    sprintf(heightMes,"0 %s", mes);
  }
  webSocket.broadcastTXT(heightMes, strlen(heightMes));
}

void processKey(int id, int val) {
  id = id + 1;
  if (val > 500) {
    bitWrite(KeyStatus, id, HIGH);
    bitWrite(ReleStatus, id, HIGH);
    sendChangesReleStatusToClients(id, LOW);
  } else {
    if (bitRead(KeyStatus, id)) {
      bitWrite(ReleStatus, id, LOW);
      sendChangesReleStatusToClients(id, HIGH);
    }
    bitWrite(KeyStatus, id, LOW);
  }
}

// сопротивление при 25 градусах по Цельсию
#define THERMISTORNOMINAL 10000
// temp. для номинального сопротивления (практически всегда равна 25 C)
#define TEMPERATURENOMINAL 25
// бета коэффициент термистора (обычно 3000-4000)
#define BCOEFFICIENT 3950
// сопротивление второго резистора
#define SERIESRESISTOR 10000

//Обработка сигнала терморезистора
void processTemp(int val) {
  //todo с терморезисторасы по Цельсию
  float average = val;
  // конвертируем значение в сопротивление
  average = 1023 / average - 1;
  average = SERIESRESISTOR / average;
  float steinhart;
  steinhart = average / THERMISTORNOMINAL; // (R/Ro)
  steinhart = log(steinhart); // ln(R/Ro)
  steinhart /= BCOEFFICIENT; // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart; // инвертируем
  steinhart -= 273.15; // конвертируем в градусы по Цельсию
  Temp1Status = steinhart;
}

//Обработка сигнала датчика уровня
void processHeight(int val) {
  if (val > 500) {
    HeightStatus = true;
  } else {
    HeightStatus = false;
  }
}

// Опрос всех подключенных датчиков и присвоение им значений в статусе
void monitorPins() {
  for(int i = 0; i < 7; i ++){
    if (i <= 4) {
      //тумблеры
      processKey(i, readMux(i));
    } else if (i == 5) {
     //терморезистор 
     processTemp(readMux(i));
    } else if (i == 6) {
     //датчик уровня
     processHeight(readMux(i));
    }
  }
  writeRelePins();
  sendUpdatesToClients();
}

void setupPins() 
{
  pinMode(latchPin, OUTPUT);
  writeRelePins();
  
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(SIG, INPUT); 
  pinMode(S0, OUTPUT); 
  pinMode(S1, OUTPUT); 
  pinMode(S2, OUTPUT); 
  pinMode(S3, OUTPUT);   

  digitalWrite(S0, LOW);
  digitalWrite(S1, LOW);
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
}

void waitForConnect() 
{
  bool isLoginGet = false;
  String ssid1;
  long milli = millis();
  while(WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
    if (millis() - milli > 10000) {
      break;
    }

    while(Serial.available()) {
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

void setup()
{
  setupPins();

  Serial.begin(115200);
  Serial.setDebugOutput(true);

  for(uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\r\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFiMulti.addAP(ssid, password);

  waitForConnect();

  if (WiFiMulti.run() != WL_CONNECTED) {
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
  } else {
    
  }

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

long lastmillis = 0;

void loop() {
  webSocket.loop();
  server.handleClient();

  //if (millis() - lastmillis > 300) {
  //    lastmillis = millis();
  //    monitorPins();
  //}
}
