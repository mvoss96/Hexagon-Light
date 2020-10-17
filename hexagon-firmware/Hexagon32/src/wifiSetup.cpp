#include <WiFi.h>
#include "config.h"
#ifdef USE_MANAGER
#include <WiFiManager.h>
#endif

void wifiSetup()
{
    uint8_t wifi_retry_count = 0; // counter for connection attemps
    while (WiFi.status() != WL_CONNECTED && wifi_retry_count < 5)
    {
        Serial.print("Not yet connected...retrying - Attempt No. ");
        Serial.println(wifi_retry_count + 1);
        WiFi.begin(
#ifndef USE_MANAGER
            WIFI_SSID, WIFI_PASSWORD
#endif
        );
        delay(3000); // time between connection attemps - should be set according to situation
        wifi_retry_count++;
    }

    if (wifi_retry_count >= 5)
    {
#ifdef USE_MANAGER
        Serial.println("no connection, starting AP");
        WiFiManager wifiManager; // WiFiManager is used to write the credentials **once**
        wifiManager.setTimeout(60);
        Serial.println("AP started");
        if (!wifiManager.startConfigPortal("HEXAGON LAMP"))
        {
            Serial.println("failed to connect and hit timeout");
            ESP.restart();
        }
#else
        Serial.print("no connection, restarting...");
        ESP.restart();

#endif
    }

    if (WiFi.waitForConnectResult() == WL_CONNECTED)
    {
        Serial.println("Connected as");
        Serial.println(WiFi.localIP());
    }
    WiFi.setHostname("Hexagon Lamp");
}

void wifiTest()
{
    uint8_t wifi_retry_count = 0;
    while (WiFi.status() != WL_CONNECTED && wifi_retry_count < 5)
    {
        wifi_retry_count++;
        Serial.println("WiFi not connected. Try to reconnect");
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_STA);
        WiFi.begin();
        delay(1000);
    }
    if (wifi_retry_count >= 5)
    {
        Serial.println("\nReboot");
        ESP.restart();
    }
}