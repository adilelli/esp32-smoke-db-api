#include "max6675.h"

#include "time.h"
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 28800;
const int   daylightOffset_sec = 0;

String startOperation;

char operationStart[100];
#include <WebServer.h>
WebServer server(80);

#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <FirebaseESP32.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "yourWifi"
#define WIFI_PASSWORD "yourPassword"

// Insert Firebase project API Key
#define API_KEY "yourApiKey"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "yourUrl" 

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;
int thermoDO = 18;
int thermoCS = 19;
int thermoCLK = 21;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

#include <ArduinoJson.h>
// JSON data buffer
StaticJsonDocument<250> jsonDocument;
char buffer[250];

void wifiConnect(){
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }

  
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void push(){
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
  // Write an Float number on the database path test/float
    if (Firebase.RTDB.pushFloatAsync(&fbdo, "test/"+ startOperation, thermocouple.readFahrenheit())){
      Serial.println("PASSED");
      // Serial.println("PATH: " + fbdo.dataPath());
      // Serial.println("TYPE: " + fbdo.dataType());
      delay(2000);
    }
    else {
      Serial.println("FAILED");
      // Serial.println("REASON: " + fbdo.errorReason());
      //delay(1000);
    }
  }
}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%F %X");
  strftime(operationStart,100, "%F %X", &timeinfo);
  startOperation = operationStart;
  push();
}

void firebaseConnect(){
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  //printLocalTime();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("MAX6675 test");
  // wait for MAX chip to stabilize
  delay(500);
  printLocalTime();
}


void create_json(char *tag, float value, char *unit) {  
  jsonDocument.clear();
  jsonDocument["type"] = tag;
  jsonDocument["value"] = value;
  jsonDocument["unit"] = unit;
  serializeJson(jsonDocument, buffer);  
}

void getTemperature() {
  Serial.println("Get temperature");
  create_json("temperature", thermocouple.readFahrenheit(), "Fahrenheit");
  server.send(200, "application/json", buffer);
}

void setupRouting(){
  server.on("/temperature", getTemperature);
  server.begin();
}


void setup() {
  wifiConnect();
  firebaseConnect();
  setupRouting();
}

void loop() {
  //set();
  push();
  server.handleClient();
}


