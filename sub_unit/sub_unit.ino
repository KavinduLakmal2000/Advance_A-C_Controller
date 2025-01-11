#include <ESP8266WiFi.h>
#include <WiFiServer.h>
#include <EEPROM.h>
#include <DHT.h>

#define SSID_ADDR 0              
#define PASSWORD_ADDR 32         
#define MAX_SSID_LENGTH 32       
#define MAX_PASS_LENGTH 32

#define DHTPIN 12//2 node//12  
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define MQ135_PIN A0
#define pir_pin 13//16 node//13
#define ldr_pin 14

#define red 4//0 node //4
#define green 5//12 node //5

int on = 180;
int off = 0;

float temperature = 0;
float humidity = 0;
float airQualityPercent = 0;
float mq135Value = 0;
boolean lights = true;
boolean pir = false;

// const char* defaultSSID = "SLT_Fiber_Optic"; 
// const char* defaultPassword = "Life1Mal7i";  

const char* defaultSSID = "4G-MIFI-IOT";
const char* defaultPassword = "Kl1234567890";

IPAddress local_IP(192, 168, 1, 252);  
IPAddress gateway(192, 168, 1, 1);    
IPAddress subnet(255, 255, 255, 0);   

WiFiServer server(80); 

void setup() {
  dht.begin();

  Serial.begin(115200);
  EEPROM.begin(64); 

  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(ldr_pin, INPUT);
  pinMode(pir_pin, INPUT);

  analogWrite(red, on);
  analogWrite(green, on);
  delay(1000);
  analogWrite(red, off);
  delay(1000);
  analogWrite(green, off);

  char storedSSID[MAX_SSID_LENGTH];
  char storedPassword[MAX_PASS_LENGTH];
  loadWiFiCredentialsFromEEPROM(storedSSID, storedPassword);

  bool isConnected = false;
  if (strlen(storedSSID) > 0 && strlen(storedPassword) > 0) {
    Serial.println("Attempting to connect with stored credentials...");
    isConnected = connectToWiFi(storedSSID, storedPassword);
  }

  if (!isConnected) {
    Serial.println("Using default WiFi credentials.");
    analogWrite(red, on);
    delay(1000);
    analogWrite(red, off);
    isConnected = connectToWiFi(defaultSSID, defaultPassword);

    if (isConnected) {
      Serial.println("Connected to WiFi using default credentials.");
    } else {
      Serial.println("Failed to connect to WiFi with default credentials.");
      analogWrite(red, on);
    }
  }

  if (isConnected) {
    Serial.print("Sub-Unit IP: ");
    Serial.println(WiFi.localIP());
    server.begin();
  }
  analogWrite(green, on);
}


void loop() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if (digitalRead(pir_pin) == HIGH){
    pir = true;
  }

  else{
    pir = false;
  }

  if(digitalRead(ldr_pin) == HIGH){
    lights = true;
  }
  else{
    lights = false;
  }

  mq135Value = analogRead(MQ135_PIN);
  airQualityPercent = map(mq135Value, 150, 900, 100, 0);

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C | Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

  Serial.print("Air Quality: ");
  Serial.print(mq135Value);
  Serial.println(" %");

  WiFiClient client = server.available(); 

  analogWrite(red, on);
  delay(50);
  analogWrite(red, off);
  delay(1000);

  if(WiFi.status() != WL_CONNECTED){
    analogWrite(green, off);
    analogWrite(red, on);
  }

  else{
    analogWrite(green, on);
  }

  if (client) {
    String request = ""; 
    unsigned long timeout = millis() + 1000; 
    while (client.connected() && millis() < timeout) {
      if (client.available()) {
        char c = client.read();
        request += c;
      }
    }

    if (request.indexOf("POST /updateWiFiCredentials") >= 0) {
      int ssidStart = request.indexOf("ssid=") + 5;
      int ssidEnd = request.indexOf("&", ssidStart);
      String newSSID = request.substring(ssidStart, ssidEnd);

      int passStart = request.indexOf("password=") + 9;
      String newPass = request.substring(passStart);

      saveWiFiCredentialsToEEPROM(newSSID, newPass);

      Serial.println("Parsed SSID: " + newSSID);
      Serial.println("Parsed Password: " + newPass);

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println();
      client.println("Credentials updated");

      delay(1000);
      ESP.restart(); 
    } 
   else {

    String data = String(temperature) + "," + String(humidity) + "," + String(airQualityPercent) + "," + String(lights ? 1 : 0) + "," + String(pir ? 1 : 0);

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println();
    client.println(data);
}


    client.stop();
  }
}


bool connectToWiFi(const char* ssid, const char* password) {
  WiFi.config(local_IP, gateway, subnet); 
  WiFi.begin(ssid, password);             

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {

    analogWrite(green, on);
    delay(500);
    analogWrite(green, off);
    delay(500);
    Serial.print(".");
    attempts++;
  }

  return WiFi.status() == WL_CONNECTED;
}


void loadWiFiCredentialsFromEEPROM(char* ssid, char* password) {
  
  for (int i = 0; i < MAX_SSID_LENGTH; i++) {
    ssid[i] = EEPROM.read(SSID_ADDR + i);
  }
  ssid[MAX_SSID_LENGTH - 1] = '\0'; 

 
  for (int i = 0; i < MAX_PASS_LENGTH; i++) {
    password[i] = EEPROM.read(PASSWORD_ADDR + i);
  }
  password[MAX_PASS_LENGTH - 1] = '\0';
}


void saveWiFiCredentialsToEEPROM(String ssid, String password) {
  for (int i = 0; i < MAX_SSID_LENGTH; i++) {
    if (i < ssid.length()) {
      EEPROM.write(SSID_ADDR + i, ssid[i]);
    } else {
      EEPROM.write(SSID_ADDR + i, 0); 
    }
  }

  for (int i = 0; i < MAX_PASS_LENGTH; i++) {
    if (i < password.length()) {
      EEPROM.write(PASSWORD_ADDR + i, password[i]);
    } else {
      EEPROM.write(PASSWORD_ADDR + i, 0); 
    }
  }

  EEPROM.commit(); 
  Serial.println("WiFi credentials saved to EEPROM.");
}
