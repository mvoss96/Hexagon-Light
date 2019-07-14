#include <WiFi.h>
#include "esp_wifi.h"
#include "config.h"

int WW = 50;
int CW = 50;
bool power = false;
char msg[10];
unsigned long refreshTimer = 0;

void WiFiSetup();
void serverSetup();
void serverRun();

void setup() {
  Serial.begin(115200);
  Serial.println("Hi");
  ledcSetup(LED_CHANNEL1, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(LED_CHANNEL2, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(LED_PIN1, LED_CHANNEL1);
  ledcAttachPin(LED_PIN2, LED_CHANNEL2);
  WiFiSetup();
  for (int i = 0; i < 2; i++) {
    ledcWrite(LED_CHANNEL2, 50);
    delay(500);
    ledcWrite(LED_CHANNEL2, 0);
    delay(500);
  }
  serverSetup();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED ) {
    WiFiSetup();
  }
  static bool firstRun = true;

  int S_CW, S_WW;
  if (WW + CW > 255) {
    float f = ((WW + CW) / 255.0);
    S_WW = WW / f;
    S_CW = CW / f;
  }
  else {
    S_CW = CW;
    S_WW = WW;
  }

  if (power) {
    ledcWrite(LED_CHANNEL1, S_WW);
    ledcWrite(LED_CHANNEL2, S_CW);
  }
  else {
    ledcWrite(LED_CHANNEL1, 0);
    ledcWrite(LED_CHANNEL2, 0);
  }
  if (millis() - refreshTimer > REFRESH_CYCLE || firstRun) {
    snprintf(msg, 10, "%d:%03d:%03d", power, S_WW, S_CW);
    refreshTimer = millis();
    firstRun = false;
  }
  serverRun();
}

//-------------------------------------

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
  switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      break;
  }
}

void WiFiSetup() {
  WiFi.disconnect();
  WiFi.onEvent(WiFiEvent);
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  esp_wifi_set_ps (WIFI_PS_NONE);
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  ledcWrite(LED_CHANNEL1, 0);
  long t = millis();
  while (WiFi.status() != WL_CONNECTED) {
    ledcWrite(LED_CHANNEL1, 50);
    delay(300);
    ledcWrite(LED_CHANNEL1, 0);
    delay(200);
    Serial.print(".");
    if (millis() - t > WIFI_TIMEOUT) {
      Serial.println("WiFi Timeout");
      delay(100);
      ESP.restart();
    }
  }
}
