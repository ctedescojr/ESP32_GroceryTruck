/**
 * @file display_manager.h
 * @brief Módulo opcional de display para o CYD (ESP32 2.8" ILI9341).
 *
 * Para ativar, adicione no platformio.ini:
 *   build_flags = -DENABLE_DISPLAY
 *
 * Dependências (lib_deps):
 *   bodmer/TFT_eSPI @ ^2.5.43
 *
 * Pinos já mapeados para o CYD — não requerem configuração extra
 * além dos build_flags do TFT_eSPI definidos no platformio.ini.
 */

#pragma once

#ifdef ENABLE_DISPLAY

#include <Arduino.h>

// --- Estados de tela ---
enum class DisplayScreen {
    BOOT,      // Splash de inicialização
    STATUS,    // IP do AP + clientes conectados
    SALES,     // Total vendido no dia + nº de vendas
    LAST_SALE, // Últimos itens vendidos
};

// Inicializa o display e exibe a tela de boot.
// Chame uma vez no setup(), APÓS initProductManager() e setupNetwork().
void initDisplay();

// Atualiza o display com dados atuais.
// Chame periodicamente no loop() ou após eventos relevantes.
void updateDisplay();

// Notifica o módulo sobre uma nova venda (atualiza tela imediatamente).
// Chame ao final de handleSaleCreateRequest(), se ENABLE_DISPLAY estiver ativo.
void displayNotifySale(float saleTotal, const String &itemsSummary);

// Alterna manualmente entre as telas (opcional, p/ botão físico).
void displayNextScreen();

#endif // ENABLE_DISPLAY