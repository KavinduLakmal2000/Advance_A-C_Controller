#define BLYNK_TEMPLATE_ID "TMPL6v1FcZP60"
#define BLYNK_TEMPLATE_NAME "IOT EcoClass"
#define BLYNK_AUTH_TOKEN "LoyG2xBpwknXDZwhaIBfRXdSs-cSUYby"

#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <FS.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define EEPROM_SIZE 96
#define SSID_ADDR 0
#define PASSWORD_ADDR 32

IPAddress subUnit1(192, 168, 1, 250);  // Sub-Unit 1 IP
IPAddress subUnit2(192, 168, 1, 251);  // Sub-Unit 2 IP
IPAddress subUnit3(192, 168, 1, 252);  // Sub-Unit 3 IP
IPAddress subUnit4(192, 168, 1, 253);  // Sub-Unit 4 IP
IPAddress DisplayUnit(192, 168, 1, 254);  // display

// char Default_ssid[] = "SLT_Fiber_Optic";
// char Default_pass[] = "Life1Mal7i";

const char* Default_ssid = "4G-MIFI-IOT";
const char* Default_pass = "Kl1234567890";

float tot_temp = 0;
float tot_humi = 0;
float final_temp = 0;
float final_humi = 0;
float temperature = 0;
float humidity = 0;
int gas = 0;
boolean lights = false;
boolean pir = false;
boolean pir1 = false;
boolean pir2 = false;
int Hours = 0;
int MiN = 0;
int sec = 0;
int subUnitCount = 0;
int notify_count = 0;
boolean Power = true;

ESP8266WebServer server(80);

const long utcOffsetInSeconds = 19800;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64  
#define OLED_RESET    -1  

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define AC_1 0 //D3
#define AC_2 14 //D5
#define Lights 12 //D6
#define Buzzer 15 //D8 
//#define TRIG_PIN D0 
//#define ECHO_PIN D7 

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  timeClient.begin();
  timeClient.update();

  pinMode(AC_1, OUTPUT);
  pinMode(AC_2, OUTPUT);
  pinMode(Lights, OUTPUT);
  pinMode(Buzzer, OUTPUT);

  digitalWrite(AC_1, LOW);
  digitalWrite(AC_2, LOW);
  digitalWrite(Lights, LOW);
  digitalWrite(Buzzer, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(18, 10);
  display.println(F("EcoClass"));
  display.setCursor(18, 30);
  display.println(F("- 2k24 -"));
  display.display();

  display.invertDisplay(true);
  delay(1000);
  display.invertDisplay(false);
  delay(2000);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(30, 0);
  display.println(F("- EcoClass -"));
  display.drawLine(0, SCREEN_HEIGHT / 5, SCREEN_WIDTH, SCREEN_HEIGHT / 5, SSD1306_WHITE);
  display.setCursor(35, 20);
  display.println("Loading...");
  display.display();

  for (int i = 0; i < 122; i++) {
    display.setCursor(i, 40);
    display.println("I");
    display.display();
    delay(5);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(30, 0);
  display.println(F("- EcoClass -"));
  display.drawLine(0, SCREEN_HEIGHT / 5, SCREEN_WIDTH, SCREEN_HEIGHT / 5, SSD1306_WHITE);
  display.setCursor(0, 30);
  display.println(F("Connecting to WiFi..."));
  display.display();

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 
  String storedSSID = readStringFromEEPROM(SSID_ADDR);
  String storedPassword = readStringFromEEPROM(PASSWORD_ADDR);

  Serial.println(storedSSID);
  Serial.println(storedPassword);

  WiFi.begin(storedSSID.c_str(), storedPassword.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    display.setCursor(attempts, 40);
    display.println("-");
    display.display();
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Blynk.config(BLYNK_AUTH_TOKEN);
    Serial.println("\nConnected to WiFi!");
    display.clearDisplay();
    display.setCursor(30, 0);
    display.println(F("- EcoClass -"));
    display.drawLine(0, SCREEN_HEIGHT / 5, SCREEN_WIDTH, SCREEN_HEIGHT / 5, SSD1306_WHITE);
    display.setCursor(0, 20);
    display.println(F("Connected to WiFi!"));
    display.setCursor(0, 30);
    display.println(WiFi.localIP());
    display.setCursor(0, 40);
    display.println(F("RSSI:"));
    display.setCursor(30, 40);
    display.println(WiFi.RSSI());
    display.setCursor(50, 40);
    display.println(F("dBm"));
    display.display();
    delay(5000);
  } 
  
  else {
    Serial.println("Failed to connect to WiFi. Using fallback credentials.");
    display.clearDisplay();
    display.setCursor(30, 0);
    display.println(F("- EcoClass -"));
    display.drawLine(0, SCREEN_HEIGHT / 5, SCREEN_WIDTH, SCREEN_HEIGHT / 5, SSD1306_WHITE);
    display.setCursor(0, 30);
    display.println(F("Failed to connect."));
    display.setCursor(0, 40);
    display.println(F("Using fallback..."));
    display.display();

    digitalWrite(Buzzer, HIGH);
    delay(500);
    digitalWrite(Buzzer, LOW);
    delay(500);
    
    WiFi.begin(Default_ssid, Default_pass);
    //Blynk.begin(BLYNK_AUTH_TOKEN, Default_ssid, Default_pass);

    int fallbackAttempts = 0;
    while (WiFi.status() != WL_CONNECTED && fallbackAttempts < 20) {
      delay(500);
      Serial.print(".");
      display.setCursor(fallbackAttempts, 50);
      display.print("-");
      display.display();
      fallbackAttempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Blynk.config(BLYNK_AUTH_TOKEN);
      Serial.println("\nConnected to WiFi using fallback credentials.");
      display.clearDisplay();
      display.setCursor(30, 0);
      display.println(F("- EcoClass -"));
      display.drawLine(0, SCREEN_HEIGHT / 5, SCREEN_WIDTH, SCREEN_HEIGHT / 5, SSD1306_WHITE);
      display.setCursor(0, 20);
      display.println(F("Connected with"));
      display.setCursor(0, 30);
      display.println(WiFi.localIP());
      display.setCursor(0, 40);
      display.println(F("RSSI:"));
      display.setCursor(30, 40);
      display.println(WiFi.RSSI());
      display.setCursor(50, 40);
      display.println(F("dBm"));
      display.display();
      delay(5000);
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("Failed to connect to WiFi with fallback credentials.");
      display.clearDisplay();
      display.setCursor(30, 0);
      display.println(F("- EcoClass -"));
      display.drawLine(0, SCREEN_HEIGHT / 5, SCREEN_WIDTH, SCREEN_HEIGHT / 5, SSD1306_WHITE);
      display.setCursor(0, 30);
      display.println(F("Failed WiFi"));
      display.setCursor(0, 20);
      display.println(F("connection."));
      display.display();
      for (;;); 
    digitalWrite(Buzzer, HIGH);
    delay(500);
    digitalWrite(Buzzer, LOW);
    delay(500);
    }
  }

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount SPIFFS");
    display.clearDisplay();
    display.setCursor(30, 0);
    display.println(F("- EcoClass -"));
    display.drawLine(0, SCREEN_HEIGHT / 5, SCREEN_WIDTH, SCREEN_HEIGHT / 5, SSD1306_WHITE);
    display.setCursor(0, 30);
    display.println(F("Failed to mount"));
    display.setCursor(0, 40);
    display.println(F("SPIFFS"));
    display.display();
    return;
  }

  Dir dir = SPIFFS.openDir("/");
  Serial.println("Listing SPIFFS files:");
  display.clearDisplay();
  display.setCursor(30, 0);
  display.println(F("- EcoClass -"));
   display.drawLine(0, SCREEN_HEIGHT / 5, SCREEN_WIDTH, SCREEN_HEIGHT / 5, SSD1306_WHITE);
   display.setCursor(0, 20);
  display.println(F("SPIFFS Files:"));
  int y = 30;
  while (dir.next()) {
    Serial.print("File: ");
    Serial.print(dir.fileName());
    Serial.print(" - Size: ");
    Serial.println(dir.fileSize());
    display.setCursor(0, y);
    display.print(dir.fileName());
    display.display();
    y += 10;
  }

  delay(3000);

  // Setup web server
server.on("/", HTTP_GET, []() {
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    server.send(404, "text/plain", "File Not Found");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
});

server.on("/home.html", HTTP_GET, []() {
  File file = SPIFFS.open("/home.html", "r");
  if (!file) {
    server.send(404, "text/plain", "File Not Found");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
});

// Serve the JavaScript file
server.on("/script.js", HTTP_GET, []() {
  File file = SPIFFS.open("/script.js", "r");
  if (!file) {
    server.send(404, "text/plain", "File Not Found");
    return;
  }
  server.streamFile(file, "application/javascript");
  file.close();
});

  server.on("/updateCredentials", HTTP_POST, []() {
    if (server.hasArg("ssid") && server.hasArg("password")) {
      String newSSID = server.arg("ssid");
      String newPassword = server.arg("password");

      sendWiFiCredentialsToSubUnits(newSSID, newPassword);

      writeStringToEEPROM(SSID_ADDR, newSSID);
      writeStringToEEPROM(PASSWORD_ADDR, newPassword);
      EEPROM.commit();

      Serial.println("New WiFi credentials received:");
      Serial.print("SSID: ");
      Serial.println(newSSID);
      Serial.print("Password: ");
      Serial.println(newPassword);

      display.clearDisplay();
      display.clearDisplay();
      display.setCursor(30, 0);
      display.println(F("- EcoClass -"));
      display.drawLine(0, SCREEN_HEIGHT / 5, SCREEN_WIDTH, SCREEN_HEIGHT / 5, SSD1306_WHITE);
      display.setCursor(0, 20);
      display.println(F("Updated WiFi"));
      display.setCursor(0, 30);
      display.println(F("SSID: "));
      display.println(newSSID);
      display.setCursor(0, 40);
      display.println(F("Pass: "));
      display.println(newPassword);
      display.display();

      server.send(200, "text/html", "<h2>WiFi Credentials Updated. Restarting...</h2>");
      digitalWrite(Buzzer, HIGH);
      delay(5000);
      ESP.restart();
    } else {
      server.send(400, "text/plain", "Missing SSID or Password");
    }
  });

  server.begin();
  Serial.println("HTTP server started");
  display.clearDisplay();
  display.setCursor(30, 0);
  display.println(F("- EcoClass -"));
  display.drawLine(0, SCREEN_HEIGHT / 5, SCREEN_WIDTH, SCREEN_HEIGHT / 5, SSD1306_WHITE);
  display.setCursor(0, 30);
  display.println(F("Server started"));
  display.display();
  digitalWrite(Buzzer, HIGH);
  delay(500);
  digitalWrite(Buzzer, LOW);
  delay(500);
  delay(5000);
}


void loop() {//-------------------------------------------------------------------------------------------------------- looooooooooooooooooop ----------------------------
  server.handleClient();
  Blynk.run();
  timeClient.update(); 
  readAllData(); 
  Serial.println(WiFi.RSSI());

  Hours = timeClient.getHours();
  MiN = timeClient.getMinutes();
  sec = timeClient.getSeconds();


if (Hours >= 8 && Hours < 18 && Power){
  Serial.println("day time");


if (pir == 1){ //------------------------------------- change this to HIGH
  digitalWrite(Lights, HIGH);
  displayOled();
  notify_count++;

  if (notify_count == 2){
  Blynk.logEvent("Movement2", "Movement detected at the class room");
  delay(1000);
  }


  if (final_temp > 31 || final_humi < 20){
    digitalWrite(Buzzer, HIGH);
    delay(200);
    digitalWrite(Buzzer, LOW);
    delay(200);
    Blynk.logEvent("environment_warning", "temp or humidity levels on warning!");
    Blynk.virtualWrite(V0, "environment_warning", "temp or humidity levels on warning!");
  }

  else{
    digitalWrite(Buzzer, LOW);
  }

  if(final_temp > 23){
    digitalWrite(AC_1, HIGH);
  }
  else{
    digitalWrite(AC_1, LOW);
  }

  if(final_temp > 26){
    digitalWrite(AC_2, HIGH);
  }
  else{
    digitalWrite(AC_2, LOW);
  }
}

else{
  display.clearDisplay();
  display.setCursor(30, 0);
  display.println(F("- EcoClass -"));
  display.drawLine(0, SCREEN_HEIGHT / 5, SCREEN_WIDTH, SCREEN_HEIGHT / 5, SSD1306_WHITE);
  display.setCursor(0, 30);
  display.println(F("Stand By"));
  display.display();
  digitalWrite(Buzzer, LOW);
  notify_count = 0;
}
 
Blynk.virtualWrite(V2, final_temp);
Blynk.virtualWrite(V4, gas);
Blynk.virtualWrite(V3, final_humi);
}

else{
  Serial.println("night time");
  if(pir == 1){
  Blynk.logEvent("Movement", "Movement detected at night time");
  Blynk.virtualWrite(V0, "Movement detected at night time");
  }
}


  delay(20);
}//-------------------------------------------------------------------------------------------------------- looooooooooooooooooop ----------------------------

BLYNK_WRITE(V1) {
  int SetPower = param.asInt();
  if (SetPower){
    Power = false;
  }
  else{
    Power = true;
  }
}

void readAllData(){
  subUnitCount = 0;
  tot_temp = 0;
  tot_humi = 0;


  String data1 = getDataFromSubUnit(subUnit1, "Sub Unit 1");
  readData(data1);
  if (!data1.startsWith("Failed") || !isnan(temperature) || !isnan(humidity) ){
    tot_temp = tot_temp + temperature;
    tot_humi = tot_humi + humidity;
    subUnitCount++; 
  }

  String data2 = getDataFromSubUnit(subUnit2, "Sub Unit 2");
  readData(data2);
  if (!data2.startsWith("Failed") || !isnan(temperature) || !isnan(humidity) ){
    tot_temp = tot_temp + temperature;
    tot_humi = tot_humi + humidity;
    subUnitCount++; 
  }

  String data3 = getDataFromSubUnit(subUnit3, "Sub Unit 3");
  readData(data3);
  if (!data3.startsWith("Failed") || !isnan(temperature) || !isnan(humidity) ){
    tot_temp = tot_temp + temperature;
    tot_humi = tot_humi + humidity;
    subUnitCount++; 
  }

  if(!data3.startsWith("Failed")){
    if (pir){
      pir1 = true;
    }
    else{
      pir1 = false;
    }
  }

  String data4 = getDataFromSubUnit(subUnit4, "Sub Unit 4");
  readData(data4);
  if (!data4.startsWith("Failed") || !isnan(temperature) || !isnan(humidity) ){
    tot_temp = tot_temp + temperature;
    tot_humi = tot_humi + humidity;
    subUnitCount++; 
  }

    if(!data4.startsWith("Failed")){
    if (pir){
      pir2 = true;
    }
    else{
      pir2 = false;
    }
  }


if (pir1 || pir2){
  pir = true;
}

final_temp = tot_temp / subUnitCount;
final_humi = tot_humi / subUnitCount;
}


void displayOled(){
  display.clearDisplay();
  display.setCursor(30, 0);
  display.println(F("- EcoClass -"));
  display.drawLine(0, SCREEN_HEIGHT / 5, SCREEN_WIDTH, SCREEN_HEIGHT / 5, SSD1306_WHITE);

  display.setCursor(0, 16);
  display.println("Status:");

  display.setCursor(0, 26);
  display.println(F("Temperature: "));
  display.setCursor(80, 26);
  display.println(final_temp); // real data 
  display.setCursor(115, 26);
  display.println("C");

  display.setCursor(0, 36);
  display.println(F("Humidity: "));
  display.setCursor(80, 36);
  display.println(final_humi); // real data 
  display.setCursor(115, 36);
  display.println("%");

  display.setCursor(0, 46);
  display.println(F("Air Quality: "));
  display.setCursor(80, 46);
  display.println(gas); // real data
  display.setCursor(115, 46);
  display.println("%");

  display.setCursor(0, 56);
  display.println(F("Lights: "));
  display.setCursor(80, 56);

  if(lights){
  display.println(F("ON"));
  }
  else{
  display.println(F("OFF"));
  }
  display.display();
}

void writeStringToEEPROM(int addr, const String &data) {
  for (int i = 0; i < data.length(); ++i) {
    EEPROM.write(addr + i, data[i]);
  }
  EEPROM.write(addr + data.length(), '\0');
}

String readStringFromEEPROM(int addr) {
  char data[32];
  int len = 0;
  unsigned char k;
  k = EEPROM.read(addr);
  while (k != '\0' && len < 31) {
    data[len++] = k;
    k = EEPROM.read(addr + len);
  }
  data[len] = '\0';
  return String(data);
}

void readData(String payload) {
    int firstComma = payload.indexOf(',');
    int secondComma = payload.indexOf(',', firstComma + 1);
    int thirdComma = payload.indexOf(',', secondComma + 1);
    int fourthComma = payload.indexOf(',', thirdComma + 1);

    if (firstComma == -1 || secondComma == -1 || thirdComma == -1 || fourthComma == -1) {
        //Serial.println("Failed to parse data");
        digitalWrite(Buzzer, HIGH);
        delay(500);
        digitalWrite(Buzzer, LOW);
        delay(500);
        return;
    }

    temperature = payload.substring(0, firstComma).toFloat();
    humidity = payload.substring(firstComma + 1, secondComma).toFloat();
    gas = payload.substring(secondComma + 1, thirdComma).toInt();
    lights = payload.substring(thirdComma + 1, fourthComma).toInt() == 1;
    pir = payload.substring(fourthComma + 1).toInt() == 1;

    Serial.print("Temp: ");
    Serial.println(temperature);
    Serial.print("Humidity: ");
    Serial.println(humidity);
    Serial.print("Gas: ");
    Serial.println(gas);
    Serial.print("Lights: ");
    Serial.println(lights);
    Serial.print("PIR: ");
    Serial.println(pir);
}


String getDataFromSubUnit(IPAddress subUnitIP, String unitName) {
  WiFiClient client;
  HTTPClient http;

  String serverURL = "http://" + subUnitIP.toString() + "/";

  if (http.begin(client, serverURL)) {
    int httpCode = http.GET();
    String payload = "";
    if (httpCode > 0) {
      payload = http.getString();
    } else {
      Serial.print("Failed to connect to ");
      Serial.print(unitName);

      payload = "Failed to connect with " + unitName;
    Blynk.virtualWrite(V0, payload);

    digitalWrite(Buzzer, HIGH);
    delay(500);
    digitalWrite(Buzzer, LOW);
    delay(500);
    }

    http.end();
    return payload;
  } 
  
  else {
    //Serial.print("Failed to start connection to ");
    //Serial.println(unitName); 

    digitalWrite(Buzzer, HIGH);
    delay(500);
    digitalWrite(Buzzer, LOW);
    delay(500);

    return "Failed to connect with " + unitName;
  }
}


void sendWiFiCredentialsToSubUnits(String ssid, String password) {
  WiFiClient client;
  HTTPClient http;

  String newCredentials = "ssid=" + ssid + "&password=" + password;
  
  // Send HTTP requests to each sub-unit to update credentials
  sendCredentialsToSubUnit(subUnit1, newCredentials);
  sendCredentialsToSubUnit(subUnit2, newCredentials);
  sendCredentialsToSubUnit(subUnit3, newCredentials);
  sendCredentialsToSubUnit(subUnit4, newCredentials);
  sendCredentialsToSubUnit(DisplayUnit, newCredentials);

}

void sendCredentialsToSubUnit(IPAddress subUnitIP, String credentials) {
  WiFiClient client;
  HTTPClient http;

  String serverURL = "http://" + subUnitIP.toString() + "/updateWiFiCredentials";  
  Serial.println("Connecting to: " + serverURL);   

  if (http.begin(client, serverURL)) {
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    Serial.println("Sending credentials: " + credentials);  

    display.clearDisplay();
    display.setCursor(30, 0);
    display.println(F("- EcoClass -"));
    display.drawLine(0, SCREEN_HEIGHT / 5, SCREEN_WIDTH, SCREEN_HEIGHT / 5, SSD1306_WHITE);
    display.setCursor(0, 20);
    display.println(F("Sending credentials"));   

    
    int httpCode = http.POST(credentials); 
    if (httpCode > 0) {
      Serial.println("Credentials sent to Sub-Unit: " + subUnitIP.toString());
      Serial.println("Response code: " + String(httpCode));  
      Serial.println("Response: " + http.getString());   

    String message = "Credentials sent to Sub-Unit: " + subUnitIP.toString();
    Blynk.virtualWrite(V0, message);
    delay(500);
    message = "Response code: " + String(httpCode);
    Blynk.virtualWrite(V0, message);

      display.setCursor(0, 30);
      display.println(subUnitIP.toString());

      display.setCursor(0, 40);
      display.println(http.getString());

    } 
    
    else {
      Serial.println("Failed to send credentials to Sub-Unit: " + subUnitIP.toString());

      display.setCursor(0, 30);
      display.println(F("Failed to send:"));
      display.setCursor(0, 40);
      display.println(subUnitIP.toString());

    String message = "Failed to send: " + subUnitIP.toString();
    Blynk.virtualWrite(V0, message);

    digitalWrite(Buzzer, HIGH);
    delay(500);
    digitalWrite(Buzzer, LOW);
    delay(500);


    }
    http.end();
    display.display(); 
    delay(100);
  }
}
