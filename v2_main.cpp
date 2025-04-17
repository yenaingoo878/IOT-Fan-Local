#include "ButtonInput.h"
#include <Arduino.h>
#include "SPIFFS.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include <AsyncTCP.h>
#include <FS.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

// Pin Definitions
#define POWER_PIN 0
#define SPEED1_PIN 1
#define SPEED2_PIN 2
#define SPEED3_PIN 3

ButtonInput BUTTON_POWER(4);
ButtonInput BUTTON_1(5);
ButtonInput BUTTON_2(6);
ButtonInput BUTTON_3(7);

// Debounce
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200;

// Status Update
unsigned long lastStatusUpdate = 0;
const unsigned long statusUpdateInterval = 3000; // every 3 seconds

// Fan State
bool currentPower = false;
int currentFanSpeed = 0;

// Network Credentials
const char* ssid = "YourSSID";             // Change this
const char* password = "YourPassword";     // Change this

// Static IP
IPAddress local_IP(192, 168, 54, 3);
IPAddress gateway(192, 168, 54, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

AsyncWebServer server(80);
WebSocketsServer webSocket(81);
Preferences preferences;

// Get Current Fan Status
String GetCurrentStatus() {
    JsonDocument doc;
    doc["power"] = currentPower;
    doc["speed"] = currentFanSpeed;
    String output;
    serializeJson(doc, output);
    return output;
}

// WebSocket Status Updates
void sendStatusUpdate() {
    if (millis() - lastStatusUpdate > statusUpdateInterval) {
        lastStatusUpdate = millis();
        String status = GetCurrentStatus();
        webSocket.broadcastTXT(status);
    }
}

// Set Fan Speed
void setFanSpeed(int speed) {
    digitalWrite(SPEED1_PIN, LOW);
    digitalWrite(SPEED2_PIN, LOW);
    digitalWrite(SPEED3_PIN, LOW);

    if (speed == 0) {
        digitalWrite(POWER_PIN, LOW);
        currentPower = false;
    } else {
        digitalWrite(POWER_PIN, HIGH);
        currentPower = true;
        switch (speed) {
            case 1: digitalWrite(SPEED1_PIN, HIGH); break;
            case 2: digitalWrite(SPEED2_PIN, HIGH); break;
            case 3: digitalWrite(SPEED3_PIN, HIGH); break;
        }
    }

    currentFanSpeed = speed;
    preferences.putInt("lastSpeed", speed);
    sendStatusUpdate();
}

// Button Check with Debounce
void checkSwitchState() {
    if (millis() - lastButtonPress < debounceDelay) {
        return;
    }

    int newSpeed = currentFanSpeed;
    if (BUTTON_POWER.isPressed() == HIGH) {
        newSpeed = 0;
    } else if (BUTTON_1.isPressed() == HIGH) {
        newSpeed = 1;
    } else if (BUTTON_2.isPressed() == HIGH) {
        newSpeed = 2;
    } else if (BUTTON_3.isPressed() == HIGH) {
        newSpeed = 0;
    }

    if (newSpeed != currentFanSpeed) {
        lastButtonPress = millis();
        setFanSpeed(newSpeed);
    }
}

// WebSocket Events
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("[%u] Connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
            webSocket.sendTXT(num, "Connected from server");
            sendStatusUpdate();
        }
        break;
        case WStype_TEXT: {
            Serial.printf("[%u] Received: %s\n", num, payload);
            String message = String((char *)(payload));

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, message);
            if (error) {
                Serial.println("JSON Parsing Failed");
                return;
            }

            if (doc["POWER"].is<bool>()) {
                bool power = doc["POWER"].as<bool>();
                int speed = doc["SPEED"].is<int>() ? doc["SPEED"].as<int>() : currentFanSpeed;

                if (!power) {
                    setFanSpeed(0);
                } else {
                    setFanSpeed(speed);
                }
            }
        }
        break;
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(POWER_PIN, OUTPUT);
    pinMode(SPEED1_PIN, OUTPUT);
    pinMode(SPEED2_PIN, OUTPUT);
    pinMode(SPEED3_PIN, OUTPUT);

    // Configure Static IP
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
        Serial.println("STA Failed to configure");
    }

    // Connect to WiFi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());

    // SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed!");
        return;
    }

    // mDNS
    if (MDNS.begin("FAN")) {
        Serial.println("mDNS responder started");
    }

    preferences.begin("FanPrefs", false);
    int savedSpeed = preferences.getInt("lastSpeed", 0);
    setFanSpeed(savedSpeed);

    // Web Files
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.html", "text/html");
    });

    server.on("/css/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/css/style.css", "text/css");
    });

    server.on("/script/socket.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/script/socket.js", "text/javascript");
    });

    server.on("/images/logo.svg", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/images/logo.svg", "image/svg+xml");
    });

    server.on("/images/power.svg", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/images/power.svg", "image/svg+xml");
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Page Not Found");
    });

    // Start Servers
    server.begin();
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi lost. Reconnecting...");
        WiFi.disconnect();
        WiFi.reconnect();
    }

    webSocket.loop();
    checkSwitchState();
    sendStatusUpdate();
    // No delay for better responsiveness
}
