#define BLYNK_TEMPLATE_ID "TMPL6dp07-fJq"
#define BLYNK_TEMPLATE_NAME "Smart Home Relay"
#define BLYNK_AUTH_TOKEN "bEjpCsgAutHvMEzV2is0oQze1mz8B9vT"

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <FirebaseESP8266.h>
#include <BlynkSimpleEsp8266.h>

#define RELAY_PIN 0
#define DOOR_BUTTON V1  // Blynk virtual pin
#define FIREBASE_DOOR_PATH "/devices/esp8266-door/door_status"

WiFiManager wifiManager;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long doorTimer = 0;
bool doorActive = false;

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // door closed

  connectToWiFi();
  setupFirebase();

  Blynk.begin(BLYNK_AUTH_TOKEN, WiFi.SSID().c_str(), WiFi.psk().c_str());
  Serial.println("Locker Node ready!");
}

void loop() {
  Blynk.run();
  listenFirebaseDoor();
  autoCloseDoor();
}

// ---- Firebase Functions ----
void connectToWiFi() {
  if (!wifiManager.autoConnect("ESP8266-Locker", "12345678")) ESP.restart();
  Serial.println("Connected to: " + WiFi.SSID());
}

void setupFirebase() {
  config.host = "https://iot-project-f14a8-default-rtdb.asia-southeast1.firebasedatabase.app";
  config.api_key = "AIzaSyAqIuNiR-vYbqcMxpVd39yQWiv6Iy6_MMQ";
  auth.user.email = "antoneyosaurav111@gmail.com";
  auth.user.password = "1234567890";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  while (auth.token.uid == "") { delay(500); Serial.print("."); }
  Serial.println("\nFirebase signed in!");
}

void listenFirebaseDoor() {
  if (Firebase.getString(fbdo, FIREBASE_DOOR_PATH)) {
    String status = fbdo.stringData();
    if (status == "OPEN" && !doorActive) {
      digitalWrite(RELAY_PIN, LOW); // open
      doorActive = true;
      doorTimer = millis();
      Blynk.virtualWrite(DOOR_BUTTON, 1);
      Serial.println("Door OPENED from Firebase");
    } else if (status == "CLOSED" && doorActive) {
      digitalWrite(RELAY_PIN, HIGH); // close
      doorActive = false;
      Blynk.virtualWrite(DOOR_BUTTON, 0);
      Serial.println("Door CLOSED from Firebase");
    }
  }
}

void autoCloseDoor() {
  if (doorActive && millis() - doorTimer > 5000) { // auto-close 5 sec
    digitalWrite(RELAY_PIN, HIGH);
    doorActive = false;
    Firebase.setString(fbdo, FIREBASE_DOOR_PATH, "CLOSED"); // sync all nodes
    Blynk.virtualWrite(DOOR_BUTTON, 0);
    Serial.println("Door auto-closed");
  }
}

// ---- Blynk control (optional manual override) ----
BLYNK_WRITE(DOOR_BUTTON) {
  if (param.asInt() == 1) {
    Firebase.setString(fbdo, FIREBASE_DOOR_PATH, "OPEN");
  } else {
    Firebase.setString(fbdo, FIREBASE_DOOR_PATH, "CLOSED");
  }
}
