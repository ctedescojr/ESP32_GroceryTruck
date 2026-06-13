/**
 * @file product_manager.h
 * @brief Gerencia toda a lógica de negócio para produtos e vendas.
 */

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

void initProductManager();

void handleGetProducts(AsyncWebServerRequest *request);
void handleGetAllProducts(AsyncWebServerRequest *request);

void handleProductCreateRequest(AsyncWebServerRequest *request);
void handleProductBulkRequest(AsyncWebServerRequest *request);

void handleGetImages(AsyncWebServerRequest *request);
void handleImageCheck(AsyncWebServerRequest *request);

void handleProductImageUploadRequest(AsyncWebServerRequest *request);
void handleProductImageUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

void handleGetSales(AsyncWebServerRequest *request);
void handleGetSalesSummary(AsyncWebServerRequest *request);
void handleSaleCreateRequest(AsyncWebServerRequest *request);

void handleSystemStorage(AsyncWebServerRequest *request);
void handleDownloadModel(AsyncWebServerRequest *request);

// The class for PUT and DELETE requests needs to be available to api_handlers
class ProductUpdateHandler : public AsyncWebHandler {
  public:
    ProductUpdateHandler();
    bool canHandle(AsyncWebServerRequest *request) override;
    void handleRequest(AsyncWebServerRequest *request) override;
    void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) override;
};