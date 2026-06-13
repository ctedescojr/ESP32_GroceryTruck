/**
 * @file main.cpp
 * @brief Ponto de entrada principal do firmware da Grocery Truck.
 * 
 * Orquestra a inicialização dos módulos de rede, sistema de arquivos,
 * e rotas da API. O loop principal é mantido limpo pois o servidor web
 * opera de forma assíncrona.
 */

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#include "network_manager.h"
#include "product_manager.h"
#include "api_handlers.h"

// Instância global do servidor
AsyncWebServer server(80);

void setup() {
    Serial.begin(115200);
    Serial.println("\nBooting ESP32_GroceryTruck v2 (Refactored)...");

    initProductManager(); // Garante que LittleFS está ok e diretórios existem
    setupNetwork();       // Configura e sobe a rede Wi-Fi
    setupApiRoutes(server); // Registra todos os endpoints da API

    server.begin();
    Serial.println("HTTP server started.");
}

void loop() {
    delay(2000); // Intencionalmente vazio. O ESPAsyncWebServer cuida de tudo em background.
}