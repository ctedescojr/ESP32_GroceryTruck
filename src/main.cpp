/**
 * @file main.cpp
 * @brief Ponto de entrada principal do firmware da Grocery Truck.
 *
 * Orquestra a inicialização dos módulos de rede, sistema de arquivos,
 * e rotas da API. O loop principal é mantido limpo pois o servidor web
 * opera de forma assíncrona.
 *
 * O módulo de display (CYD) é opcional e só é compilado/ativado quando
 * a flag ENABLE_DISPLAY é definida no platformio.ini (env grocery_truck_cyd).
 */

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

#include "api_handlers.h"
#include "network_manager.h"
#include "product_manager.h"

#ifdef ENABLE_DISPLAY
#include "display_manager.h"
#endif

// Instância global do servidor
AsyncWebServer server(80);

void setup() {
    Serial.begin(115200);
    Serial.println("\nBooting ESP32_GroceryTruck v2 (Refactored)...");

    if (!LittleFS.begin(true)) {
        Serial.println("Erro Crítico: Falha ao montar o LittleFS!");
        return;
    }

    initProductManager();   // Garante que LittleFS está ok e diretórios existem
    setupNetwork();         // Configura e sobe a rede Wi-Fi
    setupApiRoutes(server); // Registra todos os endpoints da API

#ifdef ENABLE_DISPLAY
    initDisplay(); // Inicializa o CYD após a rede estar disponível (mostra IP)
#endif

    server.begin();
    Serial.println("HTTP server started.");
}

void loop() {
#ifdef ENABLE_DISPLAY
    updateDisplay(); // Gerencia rotação de telas e refresh do CYD
#endif
    delay(250); // ESPAsyncWebServer cuida do resto em background.
                // 250ms é suficiente mesmo com display ativo (telas trocam a cada 8-10s).
}