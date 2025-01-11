#define PxMATRIX_OE_INVERT     1
#define PxMATRIX_DATA_INVERT   1
#define PxMATRIX_GAMMA_PRESET  3
#define PxMATRIX_DOUBLE_BUFFER 1

#ifdef ESP8266
#define PxMATRIX_SPI_FREQUENCY 16000000L
#endif

#include <PxMatrix.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Ticker.h>
#include <ESP8266HTTPClient.h>

// EEPROM Address Definitions
#define SSID_ADDR 0
#define PASSWORD_ADDR 32
#define MAX_SSID_LENGTH 32
#define MAX_PASS_LENGTH 32

// Default WiFi credentials
const char* defaultSSID = "4G-MIFI-IOT";
const char* defaultPassword = "Kl1234567890";

// const char* defaultSSID = "SLT_Fiber_Optic";
// const char* defaultPassword = "Life1Mal7i";


// Static IP configuration
IPAddress local_IP(192, 168, 1, 254); // Static IP for Display Unit
IPAddress gateway(192, 168, 1, 1);    // Router's gateway IP
IPAddress subnet(255, 255, 255, 0);   // Subnet mask
IPAddress dns(8, 8, 8, 8);

IPAddress subunit_IP(192, 168, 1, 253);

WiFiServer server(80); // HTTP server to receive credentials
WiFiClient client;

// NTP Configuration
const long utcOffsetInSeconds = 19800; 
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// PxMatrix Display Configuration
#define P_A   D0
#define P_B   D6
#define P_OE  D8
#define P_LAT D3
const uint8_t SPI_MOSI = D7;
const uint8_t SPI_SCK  = D5;

float temperature = 0;
float humidity = 0;
int gas = 0;
boolean lights = false;
boolean pir = false;

PxMATRIX display(32, 16, P_LAT, P_OE, P_A, P_B);
Ticker ticker;

// Function Prototypes
// void loadWiFiCredentialsFromEEPROM(char* ssid, char* password);
// void saveWiFiCredentialsToEEPROM(String ssid, String password);
// bool connectToWiFi(const char* ssid, const char* password);
// void handleClientRequests();
// void displayTime();
// void displayTemperatureHumidity(float temperature, float humidity);

void display_updater() {
    display.display(10);
}

void setup() {
    Serial.begin(115200);
    EEPROM.begin(64);

    timeClient.begin();
    timeClient.setTimeOffset(utcOffsetInSeconds);

    display.begin(4);
    display.setBrightness(7);
    ticker.attach(0.001, display_updater); 

    // Load WiFi credentials from EEPROM
    char storedSSID[MAX_SSID_LENGTH];
    char storedPassword[MAX_PASS_LENGTH];
    loadWiFiCredentialsFromEEPROM(storedSSID, storedPassword);

    // Attempt to connect using stored credentials
    bool isConnected = false;
    if (strlen(storedSSID) > 0 && strlen(storedPassword) > 0) {
        Serial.println("Attempting to connect with stored credentials...");
        isConnected = connectToWiFi(storedSSID, storedPassword);
    }

    // If connection fails, use default credentials
    if (!isConnected) {
        Serial.println("Using default WiFi credentials...");
        isConnected = connectToWiFi(defaultSSID, defaultPassword);
        if (isConnected) {
            Serial.println("Connected using default credentials.");

              display.clearDisplay();
              display.setTextColor(0xFF);
              display.setCursor(5, 0);
              display.print("WiFi");
              display.setCursor(10, 8);
              display.print(":)");
              display.showBuffer();
              delay(3000);

        } else {
            Serial.println("Failed to connect using default credentials.");
              display.clearDisplay();
              display.setTextColor(0xFF);
              display.setCursor(5, 0);
              display.print("WiFi");
              display.setCursor(10, 8);
              display.print(":(");
              display.showBuffer();
              delay(3000);
        }
    }

    // Start HTTP server if connected
    if (isConnected) {
        Serial.print("Display Unit IP: ");
        Serial.println(WiFi.localIP());
        server.begin();
    } 

  while (!timeClient.update()) {
        delay(500);
        Serial.println("Waiting for NTP synchronization...");
    }
    Serial.println("NTP synchronization successful!");

    
}

void loop() {
    handleClientRequests();
    timeClient.update();

    String data1 = getDataFromSubUnit(subunit_IP, "Sub Unit");
    handleClientRequests();
    readData(data1);
    handleClientRequests();

    if(pir){
    displayTime();

    handleClientRequests();

    delay(6000);
    handleClientRequests();

    displayValues(temperature, humidity, gas);

    handleClientRequests();
    }

    else{
      display.clearDisplay();
    }

   
}


void displayValues(int temperature, int humidity, int airQuality) {
    display.clearDisplay();
    display.setTextColor(0xFF);

    char tempString[16];
    char humidityString[16];
    char airQualityString[8];

    snprintf(tempString, sizeof(tempString), "T:%dC", temperature);
    snprintf(humidityString, sizeof(humidityString), "H:%d%%", humidity);
    snprintf(airQualityString, sizeof(airQualityString), "%d%%", airQuality);

    display.setCursor(0, 0);
    display.print(tempString);
    display.setCursor(0, 8);
    display.print(humidityString);
    display.showBuffer();
    delay(2000);

    display.clearDisplay();
    display.setTextColor(0xFF);
    display.setCursor(0, 0);
    display.print("Air:");
    display.setCursor(8, 8);
    display.print(airQualityString);
    display.showBuffer();
    delay(2000);
}




bool connectToWiFi(const char* ssid, const char* password) {
    WiFi.config(local_IP, gateway, subnet, dns); 
    delay(100);
    WiFi.begin(ssid, password);             

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    return WiFi.status() == WL_CONNECTED;
}


void handleClientRequests() {
    WiFiClient client = server.available();
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

            display.clearDisplay();
            display.setTextColor(0xFF);
            display.setCursor(0, 0);
            display.print("WiFi");
            display.setCursor(0, 8);
            display.print("Set");
            display.showBuffer();

            delay(4000);
            ESP.restart();
        } else {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/plain");
            client.println();
            client.println("Display Unit is active.");
        }
        client.stop();
    }
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
        EEPROM.write(SSID_ADDR + i, (i < ssid.length()) ? ssid[i] : 0);
    }
    for (int i = 0; i < MAX_PASS_LENGTH; i++) {
        EEPROM.write(PASSWORD_ADDR + i, (i < password.length()) ? password[i] : 0);
    }
    EEPROM.commit();
    Serial.println("WiFi credentials saved to EEPROM.");
}

void displayTime() {
    
    int hours = timeClient.getHours();
    int minutes = timeClient.getMinutes();
    int seconds = timeClient.getSeconds();

    char timeString[6];
    snprintf(timeString, sizeof(timeString), "%02d:%02d", hours, minutes);

    display.clearDisplay();
    display.setTextColor(0xFF);
    display.setCursor(4, 0);
    display.print("TIME");
    display.setCursor(1, 8);
    display.print(timeString);
    display.showBuffer();
}


void readData(String payload) {
    int firstComma = payload.indexOf(',');
    int secondComma = payload.indexOf(',', firstComma + 1);
    int thirdComma = payload.indexOf(',', secondComma + 1);
    int fourthComma = payload.indexOf(',', thirdComma + 1);

    if (firstComma == -1 || secondComma == -1 || thirdComma == -1 || fourthComma == -1) {
        Serial.println("Failed to parse data");
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
    }

    http.end();
    return payload;
  } else {
    Serial.print("Failed to start connection to ");
    Serial.println(unitName);
    return "Failed to connect with " + unitName;
  }
}




