#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

// OLED Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// GPS serial setup (connect RX/TX to D6 and D7)
static const int RXPin = D6, TXPin = D7;
static const uint32_t GPSBaud = 9600;

TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

// WiFi and MQTT settings
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";
const char* mqtt_server = "broker.hivemq.com"; // Change to your MQTT broker if needed

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
char msg[256];

// Setup WiFi connection
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Reconnect to MQTT broker if connection is lost
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe to topics or publish messages here if needed
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Publish GPS data to MQTT as JSON
void publishGpsData() {
  if (gps.location.isValid()) {
    StaticJsonDocument<200> doc;

    // Add data to JSON object
    doc["lat"] = gps.location.lat();
    doc["lng"] = gps.location.lng();
    doc["speed"] = gps.speed.kmph();
    doc["datetime"] = String(gps.date.year()) + "-" + String(gps.date.month()) + "-" + String(gps.date.day()) + " " +
                      String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second());

    // Serialize JSON object to a string
    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);

    // Publish the JSON string to MQTT
    client.publish("gps/data", jsonBuffer);
  }
}

// Display GPS data on OLED
void displayGpsInfo() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  if (gps.location.isValid()) {
    display.setCursor(0, 0);
    display.print("Lat: "); display.println(gps.location.lat(), 6);
    display.print("Lng: "); display.println(gps.location.lng(), 6);
    display.print("Speed: "); display.print(gps.speed.kmph()); display.println(" km/h");
    display.print("Date: ");
    display.print(gps.date.year()); display.print("-");
    display.print(gps.date.month()); display.print("-");
    display.println(gps.date.day());
    display.print("Time: ");
    display.print(gps.time.hour()); display.print(":");
    display.print(gps.time.minute()); display.print(":");
    display.println(gps.time.second());
  } else {
    display.setCursor(0, 0);
    display.println("Waiting for GPS data...");
  }

  display.display();
}

void setup() {
  Serial.begin(115200);
  ss.begin(GPSBaud);
  
  // Setup OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();

  // Initialize WiFi and MQTT
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  // Ensure MQTT connection
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Get GPS data
  while (ss.available() > 0) {
    gps.encode(ss.read());
  }

  // Publish GPS data every 5 seconds
  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    publishGpsData();
    displayGpsInfo();
  }
}
