/**
 * @file api_handlers.cpp
 * @brief Implementação da configuração das rotas da API.
 */

#include "api_handlers.h"
#include "product_manager.h"
#include "utils.h"
#include <LittleFS.h>

void setupApiRoutes(AsyncWebServer &server) {
    // GET Endpoints
    server.on("/api/products", HTTP_GET, handleGetProducts);
    server.on("/api/products/all", HTTP_GET, handleGetAllProducts);
    server.on("/api/images", HTTP_GET, handleGetImages);
    server.on("/api/images/check", HTTP_GET, handleImageCheck);
    server.on("/api/sales", HTTP_GET, handleGetSales);
    server.on("/api/sales/summary", HTTP_GET, handleGetSalesSummary);
    server.on("/api/system/storage", HTTP_GET, handleSystemStorage);
    server.on("/modelo", HTTP_GET, handleDownloadModel);

    // POST Endpoints with Body parsing lambda
    auto bodyConcatenator = [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (index == 0) {
            String *body = new String();
            body->reserve(total);
            request->_tempObject = body;
        }
        String *body = (String *)request->_tempObject;
        body->concat((const char *)data, len);
    };

    server.on("/api/products/bulk", HTTP_POST, handleProductBulkRequest, NULL, bodyConcatenator);
    server.on("/api/products", HTTP_POST, handleProductCreateRequest, NULL, bodyConcatenator);
    server.on("/api/sales", HTTP_POST, handleSaleCreateRequest, NULL, bodyConcatenator);

    // Image Upload Endpoint
    server.on("/api/products/image/upload", HTTP_POST, handleProductImageUploadRequest, handleProductImageUpload);

    // Dynamic Product PUT/DELETE Endpoint
    server.addHandler(new ProductUpdateHandler());

    // Basic routes and CORS fallback
    // Removido o bloqueio do favicon para que o ESPAsyncWebServer o sirva via LittleFS

    server.onNotFound([](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_OPTIONS) {
            AsyncWebServerResponse *response = request->beginResponse(200);
            addCorsHeaders(response);
            request->send(response);
        } else {
            if (request->url() == "/") {
                request->send(LittleFS, "/index.html", "text/html");
            } else if (request->url() == "/admin") {
                request->send(LittleFS, "/admin.html", "text/html");
            } else if (LittleFS.exists(request->url())) {
                AsyncWebServerResponse *response = request->beginResponse(LittleFS, request->url(), String(), false);
                response->addHeader("Cache-Control", "max-age=3600");
                request->send(response);
            } else {
                request->send(404, "text/plain", "Not found");
            }
        }
    });
}