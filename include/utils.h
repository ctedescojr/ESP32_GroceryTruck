/**
 * @file utils.h
 * @brief Declarações de funções utilitárias gerais para o projeto.
 */

#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

String generateSlug(String input);
void addCorsHeaders(AsyncWebServerResponse *response);