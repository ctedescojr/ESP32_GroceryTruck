/**
 * @file network_manager.cpp
 * @brief Implementação do gerenciador de conexão Wi-Fi.
 */

#include "network_manager.h"

const char *AP_SSID     = "GroceryTruck";
const char *AP_PASSWORD = "fiorino123";
IPAddress   AP_IP(192, 168, 4, 1);
IPAddress   AP_GATEWAY(192, 168, 4, 1);
IPAddress   AP_SUBNET(255, 255, 255, 0);

void setupNetwork() {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
}