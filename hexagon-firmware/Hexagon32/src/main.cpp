#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"
#define PWM_RESOLUTION 8 //8bit for 0-255 do not change!

uint8_t color = 0;
uint8_t brightness = 0;
bool power = true;
char msgBuffer[128];

void wifiSetup();
void wifiTest();
void serverSetup();
void serverRun();
void spiffsSetup();

void setup()
{
  Serial.begin(115200);
  Serial.println("HEXAGON LAMP");
  Serial.println("reading from EEPROM...");
  EEPROM.begin(3);
  color = EEPROM.read(0);
  brightness = EEPROM.read(1);
  power = EEPROM.read(2);
  Serial.print("color:");
  Serial.print(color);
  Serial.print("; brightness:");
  Serial.print(brightness);
  Serial.print("; power:");
  Serial.println(power);
  //configure PWM output pins
  ledcSetup(LED_CHANNEL1, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(LED_CHANNEL2, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(LED_PIN1, LED_CHANNEL1);
  ledcAttachPin(LED_PIN2, LED_CHANNEL2);

  wifiSetup();
  spiffsSetup();
  serverSetup();
}

void loop()
{
  static uint8_t last_color = color;
  static uint8_t last_brightness = brightness;
  static bool last_power = power;
  if (last_color != color)
  {
    EEPROM.write(0, color);
    EEPROM.commit();
  }
  if (last_brightness != brightness)
  {
    EEPROM.write(1, brightness);
    EEPROM.commit();
  }
  if (last_power != power)
  {
    EEPROM.write(2, power);
    EEPROM.commit();
  }
  wifiTest();
  serverRun();

  if (power)
  {
    ledcWrite(LED_CHANNEL1, (color / 100.0) * brightness);
    ledcWrite(LED_CHANNEL2, (1 - color / 100.0) * brightness);
  }
  else
  {
    ledcWrite(LED_CHANNEL1, 0);
    ledcWrite(LED_CHANNEL2, 0);
  }
}

//-------------------------------------END
