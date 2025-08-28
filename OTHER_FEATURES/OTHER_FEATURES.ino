#define BLYNK_TEMPLATE_ID "TMPL6dp07-fJq"
#define BLYNK_TEMPLATE_NAME "Smart Home Relay"
#define BLYNK_AUTH_TOKEN "9as_L1T2zilR6Z66elR9I4pWgaxofwH9"

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <FirebaseESP8266.h>
#include <BlynkSimpleEsp8266.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 4
#define RST_PIN 5
#define FIREBASE_DOOR_PATH "/devices/esp8266-door/door_status"
#define FIREBASE_LOG_PATH  "/devices/esp8266-door/door_log"

MFRC522 rfid(SS_PIN, RST_PIN);
WiFiManager wifiManager;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Allowed UID
byte allowedUID[4] = {0x37, 0x24, 0x49, 0x01};

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();

  connectToWiFi();
  setupFirebase();

  Blynk.begin(BLYNK_AUTH_TOKEN, WiFi.SSID().c_str(), WiFi.psk().c_str());
  Serial.println("RFID Node ready!");
}

void loop() {
  Blynk.run();
  checkRFIDCard();
}

bool isCardMatched() {
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] != allowedUID[i]) return false;
  }
  return true;
}

void checkRFIDCard() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uidStr = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] < 0x10) uidStr += "0";
      uidStr += String(rfid.uid.uidByte[i], HEX);
    }
    uidStr.toUpperCase();

    if (isCardMatched()) {
      Serial.println("Authorized Card: " + uidStr);
      Firebase.setString(fbdo, FIREBASE_DOOR_PATH, "OPEN");  // open door
      logRFIDEvent("Authorized", uidStr);
    } else {
      Serial.println("Unauthorized Card: " + uidStr);
      logRFIDEvent("Unauthorized", uidStr);
    }

    rfid.PICC_HaltA();
    delay(500);
  }
}

void logRFIDEvent(String status, String uidStr) {
  String entry = status + " - UID: " + uidStr + " at " + String(millis());
  if (Firebase.pushString(fbdo, FIREBASE_LOG_PATH, entry))
    Serial.println("Logged: " + entry);
  else
    Serial.println("Firebase Error: " + fbdo.errorReason());
}

void connectToWiFi() {
  if (!wifiManager.autoConnect("ESP8266-RFID", "12345678")) ESP.restart();
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
