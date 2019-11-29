#include <Adafruit_Sensor.h>
#include <DHT.h>
//#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
 
ESP8266WiFiMulti wifiMulti;


#define DHT_PIN D7
#define ANALOG_PIN A0

#define LED_PIN D3
#define FUN_PIN D4
#define TERM_PIN D5
#define PUMP_PIN D6
#define TWO_HOURS 7200000

int motors[4] = {255, 255, 255, 255};
int sensors[4] = {255, 255, 255, 255};
bool flagDHT = false;
bool flagPump = false;


char str[15];
char rs[15];
unsigned int timePump = 0;
unsigned long previousMillisPump = millis();


const char *essid1="OdinokyMacho";
const char *key1="314159265";
const char* essid2="hackcafe";
const char* key2="e326200a959b";
const char *key="";
const char * essid3="HUAWEI-54t2";
const char * key3="48575443F9A1E896";


// Initialize the client library
WiFiClient client;

const char * host = "greenbox-server.herokuapp.com";
String url = "/synchronize_sensors";
//const char* host = "google.com";
//String url = "/accounts/ClientLogin";
 
DHT dht(DHT_PIN, DHT11);

void setup() {
 WiFi.mode(WIFI_STA);

wifiMulti.addAP(essid1,key1);
wifiMulti.addAP(essid2,key2);
wifiMulti.addAP(essid3,key3);

  Serial.begin(115200);
  Serial.print("Connecting to ");
  Serial.println(essid1);
  //WiFi.disconnect();
  while(wifiMulti.run() != WL_CONNECTED)
  {
      delay(500);
      Serial.print(".");
      //Serial.print(wifiMulti.run());

  }
//    WiFi.mode(WIFI_STA);
//  WiFi.begin(essid1,key1);
//  while(WiFi.status() != WL_CONNECTED)
//  {
//      delay(500);
//      Serial.print(".");
//      //Serial.print(wifiMulti.run());
//
//  }
  Serial.println("WiFi connected");
//  Serial.println(WiFi.localIP());
  dht.begin();  
  Serial.println("Ready!");
}

void loop() {
  
  if(!client.connect(host ,80))
  {
      Serial.println("connection failed");
      return;
  } 
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  readSensors();
  printSerial();
  sendAndGetInfo();
//  getInfo();
//  writeMotors();
  readSerial();
  delay(3000);
}

void readSensors() {
  //whatever we set on ONLY analog in
  sensors[1] = analogRead(ANALOG_PIN);
  sensors[2] = (dht.readTemperature());
  sensors[3] = (dht.readHumidity());
}

void printSerial() {
  sprintf(str, "%d,%d,%d,%d", -1, sensors[1], sensors[2], sensors[3]);
  Serial.println(str);
}

void writeMotors() {
  writeLEDAndFunAndTERM();
  writePump();
}

void writeLEDAndFunAndTERM() {
  analogWrite(TERM_PIN, motors[0]);
  analogWrite(LED_PIN, motors[1]);
  analogWrite(FUN_PIN, motors[2]);
}

void writePump() {
//    digitalWrite(PUMP_PIN, true);

  if(millis() > previousMillisPump) {
    flagPump = !flagPump;
    digitalWrite(PUMP_PIN, flagPump);
    if(flagPump) {
      previousMillisPump = millis() + timePump;
    } else {
      previousMillisPump = millis() + (TWO_HOURS - timePump);
    }
  }
}

void readSerial() {
  if (Serial.available() > 0) {    
    for (byte i = 0; i < 15; i++)
    {
        rs[i] = Serial.read();
    }
    Serial.flush();
    String res = String(rs);
    for(byte i = 0; i < 3; i++){
      motors[i] = res.substring(0, res.indexOf(",")).toInt();
      res = res.substring(res.indexOf(",")+1, res.length());
      Serial.println(motors[i]);
    }
    motors[3] = res.substring(0, res.length()).toInt();
    Serial.println(motors[3]);
  }
}


String sendAndGetInfo(){
  const size_t capacity = JSON_OBJECT_SIZE(4)+60;
  DynamicJsonDocument doc(capacity);
  doc["light"] = sensors[0];
  doc["soil_humidity"] = sensors[1];
  doc["temperature"] = sensors[2];
  doc["air_humidity"] = sensors[3];
  
  Serial.print("Sending: ");
  serializeJson(doc, Serial);
  Serial.println();
  client.print(F("POST "));
  client.print(url);
  client.println(F(" HTTP/1.1"));
  client.print(F("Host: "));
  client.println(host);
//  client.println(F("POST /synchronize_sensors HTTP/1.1"));
//  client.println(F("Host: greenbox-server.herokuapp.com"));
  client.println(F("Accept: */*"));
  client.print(F("Content-Length: "));
  client.println(measureJsonPretty(doc));
  client.println(F("Content-Type: application/json"));
  client.println(F("Connection: close"));
  client.println();
  serializeJsonPretty(doc, client);
  unsigned long timeout = millis();
  while(client.available()==0)
  {
      if(millis()-timeout>5000)
      {
          Serial.println(">>> Client Timeout !");
          client.stop();
          return "";
      }
  }

  DynamicJsonDocument resp(capacity);
  String res = "";
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
        Serial.println(F("Invalid response"));
        return "";
  }
  DeserializationError error = deserializeJson(resp, client);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return "";
  }
  motors[0] = resp["servomotor"].as<int>();
  motors[1] = resp["light"].as<int>();
  motors[2] = resp["fun"].as<int>();
  motors[3] = resp["waterpump"].as<int>();

  Serial.println(F("Response:"));
  Serial.println(motors[0]);
  Serial.println(motors[1]);
  Serial.println(motors[2]);
  Serial.println(motors[3]);
  client.stop();
  delay(3000);
  return res;
}
