#define BLYNK_TEMPLATE_ID "TMPL6dp07-fJq"
#define BLYNK_TEMPLATE_NAME "Smart Home Relay"
#define BLYNK_AUTH_TOKEN "bEjpCsgAutHvMEzV2is0oQze1mz8B9vT"


#include <ESP8266WiFi.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <FirebaseESP8266.h>     // firebase
#include <BlynkSimpleEsp8266.h>  // blynk

// Your Firebase project credentials
#define FIREBASE_HOST "https://iot-project-f14a8-default-rtdb.asia-southeast1.firebasedatabase.app"  // e.g. "myproject-default-rtdb.firebaseio.com"
#define FIREBASE_API_KEY "AIzaSyAqIuNiR-vYbqcMxpVd39yQWiv6Iy6_MMQ"                                   // from Firebase Console → Project settings → Web API Key
#define FIREBASE_EMAIL "antoneyosaurav111@gmail.com"
#define FIREBASE_PASSWORD "1234567890"

#define RESET_BUTTON V0
#define DOOR_BUTTON V1

#define RELAY_PIN 0

unsigned long doorTimer = 0;
bool doorActive = false;

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

WiFiManager wifiManager;


void setup() {
  Serial.begin(115200);

  connectToWiFi();     // Connect to the wifi
  setupFirebase();     // Step 2: Firebase
  updateWiFiStatus();  // Step 3: Write initial status

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  

  // Connect to Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, WiFi.SSID().c_str(), WiFi.psk().c_str());

  Blynk.virtualWrite(RESET_BUTTON, 0);
  Serial.println("Blynk connected!");
}

void loop() {
  Blynk.run();
  
  // Auto turn-off relay after 5 seconds
  if (doorActive && millis() - doorTimer >= 5000) {
    digitalWrite(RELAY_PIN, HIGH);
    doorActive = false;
    logDoorEvent("Door CLOSED (auto)");
    Blynk.virtualWrite(DOOR_BUTTON, 0);  // turn button off in app
  }
}

void logDoorEvent(const char* event) {
  String path = "/devices/esp8266/door_log";
  String timestamp = String(millis());
  if (!Firebase.pushString(fbdo, path, String(event) + " at " + timestamp)) {
    Serial.println("Firebase logging error: " + fbdo.errorReason());
  } else {
    Serial.println("Logged door event: " + String(event));
  }
}

// ----------- Firebase Setup Function ------------
void setupFirebase() {
  // Configure Firebase
  config.host = FIREBASE_HOST;
  config.api_key = FIREBASE_API_KEY;

  // Auth with email & password
  auth.user.email = FIREBASE_EMAIL;
  auth.user.password = FIREBASE_PASSWORD;

  // Connect to Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Wait for sign in
  Serial.println("Signing in to Firebase...");
  while (auth.token.uid == "") {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nFirebase signed in successfully!");
}

// Function to handle Wi-Fi connection
void connectToWiFi() {
  // Try to connect to saved Wi-Fi, otherwise start AP portal
  if (!wifiManager.autoConnect("ESP8266-Setup", "12345678")) {
    Serial.println("Failed to connect and hit timeout");
    ESP.restart();  // restart if cannot connect
    delay(1000);
  }

  Serial.println("Connected to: " + WiFi.SSID());
  Serial.println("IP Address: " + WiFi.localIP().toString());
}

// ----------- Update Wi-Fi Status to Firebase ------------
void updateWiFiStatus() {
  String status = WiFi.isConnected() ? "online" : "offline";
  if (Firebase.setString(fbdo, "/devices/esp8266/status", status)) {
    Serial.println("Updated status: " + status);
  } else {
    Serial.println("Firebase Error: " + fbdo.errorReason());
  }
}

// Virtual button callback
BLYNK_WRITE(RESET_BUTTON) {
  int pinValue = param.asInt();  // 1 = pressed, 0 = released
  if (pinValue == 1) {
    Serial.println("Blynk button pressed! Resetting Wi-Fi...");
    Blynk.virtualWrite(RESET_BUTTON, 0);
    Firebase.setString(fbdo, "/devices/esp8266/status", "Offline");

    wifiManager.resetSettings();
    Serial.println("Blynk button pressed! Resetting Wi-Fi...");
    ESP.restart();
  }
}

// Door control
BLYNK_WRITE(DOOR_BUTTON) {
  if (param.asInt() == 1) {
    digitalWrite(RELAY_PIN, LOW);
    doorActive = true;
    doorTimer = millis();
    logDoorEvent("Door OPEN");
  } else {
    digitalWrite(RELAY_PIN, HIGH);
    doorActive = false;
    logDoorEvent("Door CLOSED");
  }
}
