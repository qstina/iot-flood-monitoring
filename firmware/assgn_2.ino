#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// --- Network & MQTT Config ---
const char* ssid = "cslab";
const char* password = "aksesg31";
const char* mqtt_server = "34.31.3.122"; 
const int mqtt_port = 8883; 
const char* client_id = "esp32_flood_monitor";
const char* topic = "projects/sublime-lodge-481702-c8/topics/assgn-2";

// --- PASTE YOUR ca.crt CONTENT HERE ---
// Run 'cat /etc/mosquitto/certs/ca.crt' in your VM to get this text
const char* root_ca = \

"-----BEGIN CERTIFICATE-----\n" \
"MIIDCTCCAfGgAwIBAgIUfA4OU1gClaAo11P8MtNP20dy3jowDQYJKoZIhvcNAQEL\n"
"BQAwFDESMBAGA1UEAwwJTXlMb2NhbENBMB4XDTI2MDExNjEzMjQwNVoXDTM2MDEx\n"
"NDEzMjQwNVowFDESMBAGA1UEAwwJTXlMb2NhbENBMIIBIjANBgkqhkiG9w0BAQEF\n"
"AAOCAQ8AMIIBCgKCAQEA4ddFcN/37gt2JEwDj1Tn6GsKCplCMhZpMtBcPDDtmTb8\n"
"rseN6XQGuQ2r+CCFwaYdHx/mU9Ofb50MwBcODS+6+tdTojhKVZ6VUNH4bLDyeeAY\n"
"WAf4QyaQgI4YnoK/tTlUodfvr8acnrI/VQjSf96JVpLDlbvU2qTjueDQpFZvCpS2\n"
"ICZKg++hOYsixRKWL0K8AIAWbpJkIK+NizJBUP4vC4UwURlKMrsszcKWj0Xy4+ce\n"
"LUCvOf3APM3v3dAVSsNp2kCopIH0p2JoG9Q45hvuSUxnexsh0VJMYuE3TmE6qkGo\n"
"MLsoxI9Z301G1XX2CRDiffHDU92R5NMbu6pH4VabMQIDAQABo1MwUTAdBgNVHQ4E\n"
"FgQU8iPWdpV64D0X6ecGAt/P23GnxxUwHwYDVR0jBBgwFoAU8iPWdpV64D0X6ecG\n"
"At/P23GnxxUwDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAD8bN\n"
"RCUzNTknV/RpyLKcwqLgRwwpgOIeQxTzasiA0z+nwsDQvqtsezCvOewgDOByUI4m\n"
"qjMjqShzvI96PHL5r98kJKlAses7tS4LeK3GfrloAGQD6T4SKXT8VHFn68LfS6Ar\n"
"pbjMRbEioqN7QA3jPM59nRgc/pNVu+uYbexZ6kgORb2eNI5utxiW9TjGCcwCYSJM\n"
"6pOPzC0DSqYCJcFcUUG4jdjo/EmAAjucWPfhKd9fa+oJI5AAIaov2ZhKMt5A/oN4\n"
"aHGHYVIAAD3U7/Q9mslUsZlSRi5VXZ0Q0e1dw1mj34m24V0J58XxaooQZWRNc6mJ\n"
"FUIunDiIF8jwxr0lYg==\n" \
"-----END CERTIFICATE-----";

// --- Pin Definitions ---
const int rainPin = 34;  // Raindrops AO
const int trigPin = 5;   // Ultrasonic Trig
const int echoPin = 18;  // Ultrasonic Echo

WiFiClientSecure espClient;
PubSubClient client(espClient);

float getWaterDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  return duration * 0.034 / 2;
}

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  
  espClient.setCACert(root_ca);
  client.setServer(mqtt_server, mqtt_port);
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect(client_id)) {
      Serial.println("Connected to Secure MQTT");
    } else {
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  int rainValue = analogRead(rainPin);     // 4095 = Dry, <2000 = Raining
  float waterDistance = getWaterDistance(); // High = Safe, Low = Flood Risk

  StaticJsonDocument<256> doc;
  doc["device_id"] = "FloodNode_01";
  doc["rain_intensity"] = rainValue;
  doc["water_level_cm"] = waterDistance;
  
  // Logical Alert Level
  if(rainValue < 2500 && waterDistance < 20.0) {
    doc["alert_level"] = "CRITICAL";
  } else if (rainValue < 2500) {
    doc["alert_level"] = "WARNING_RAIN";
  } else {
    doc["alert_level"] = "STABLE";
  }

  char buffer[256];
  serializeJson(doc, buffer);
  client.publish(topic, buffer);

  Serial.print("Data Sent: "); Serial.println(buffer);
  delay(10000); 
}