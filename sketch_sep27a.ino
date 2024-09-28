#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// OLED display parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// GPS on D6 (RX) and D7 (TX)
static const int RXPin = D7, TXPin = D6;
static const uint32_t GPSBaud = 9600;

// Create GPS and MQTT client objects
TinyGPSPlus gps;
SoftwareSerial gpsSerial(RXPin, TXPin);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// WiFi and MQTT server details
const char* ssid = "DPE_network";       // Replace with your WiFi SSID
const char* password = "jangansombong"; // Replace with your WiFi password
const char* mqttServer = "192.168.1.23"; // Replace with your MQTT server address
const int mqttPort = 1883;             // Default MQTT port

// Variables for time tracking
unsigned long lastPublishTime = 0;
const unsigned long publishInterval = 5000; // 5 seconds

void setup() {
  // Start serial for debugging
  Serial.begin(115200);

  // Start serial communication with GPS module
  gpsSerial.begin(GPSBaud);

  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Connect to WiFi
  connectToWiFi();
  
  // Setup MQTT server
  mqttClient.setServer(mqttServer, mqttPort);
}

void loop() {
  // Handle GPS data
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
    displayGPSData();
  }

  // Check if it's time to publish data
  if (millis() - lastPublishTime >= publishInterval) {
    publishGPSData();
    lastPublishTime = millis();
  }

  // Reconnect to MQTT if disconnected
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();
}

// Function to display GPS data on OLED
void displayGPSData() {
  display.clearDisplay();
  display.setCursor(0, 0);
  
  if (gps.location.isValid()) {
    display.print(F("Lat: "));
    display.println(gps.location.lat(), 6);
    display.print(F("Lng: "));
    display.println(gps.location.lng(), 6);
    display.print(F("Speed: "));
    display.println(gps.speed.kmph(), 2);
  } else {
    display.println(F("Location Invalid"));
  }

  if (gps.date.isValid() && gps.time.isValid()) {
    display.print(F("Date: "));
    display.print(gps.date.day());
    display.print(F("/"));
    display.print(gps.date.month());
    display.print(F("/"));
    display.println(gps.date.year());

    display.print(F("Time: "));
    if (gps.time.hour() < 10) display.print(F("0"));
    display.print(gps.time.hour());
    display.print(F(":"));
    if (gps.time.minute() < 10) display.print(F("0"));
    display.print(gps.time.minute());
    display.print(F(":"));
    if (gps.time.second() < 10) display.print(F("0"));
    display.println(gps.time.second());
  } else {
    display.println(F("Date/Time Invalid"));
  }

  display.display();
}

// Function to publish GPS data to MQTT server
void publishGPSData() {
  if (gps.location.isValid() && gps.speed.isValid() && gps.date.isValid() && gps.time.isValid()) {
    String lat = String(gps.location.lat(), 6);
    String lng = String(gps.location.lng(), 6);
    String speed = String(gps.speed.kmph(), 2);
    String datetime = String(gps.date.year()) + "-" + String(gps.date.month()) + "-" + String(gps.date.day()) + " " +
                      String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second());

    String payload = "Lat: " + lat + ", Lng: " + lng + ", Speed: " + speed + " km/h, DateTime: " + datetime;

    if (mqttClient.publish("gps/location", payload.c_str())) {
      Serial.println("GPS data published: " + payload);
    } else {
      Serial.println("Failed to publish GPS data");
    }
  }
}

// Function to connect to WiFi
void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Wifi Connected!");
  Serial.println("\nWiFi connected");
}

// Function to reconnect to MQTT server
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect("ESP8266Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
