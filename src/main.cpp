#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

const char *AP_SSID = "GroceryTruck";
const char *AP_PASSWORD = "fiorino123";
IPAddress AP_IP(192, 168, 4, 1);
IPAddress AP_GATEWAY(192, 168, 4, 1);
IPAddress AP_SUBNET(255, 255, 255, 0);

AsyncWebServer server(80);
File uploadFile;

// Utility: Normalize and Generate Slug
String generateSlug(String input) {
    String slug = "";
    input.toLowerCase();
    
    for (size_t i = 0; i < input.length(); i++) {
        char c = input[i];
        // Basic unaccent mapping (works mostly for UTF-8 encoded single-byte extensions or by ignoring multibyte)
        // Since UTF-8 takes multiple bytes for accents, a simple replace is safer.
        // For robustness, we will just keep a-z, 0-9, and replace spaces with _.
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            slug += c;
        } else if (c == ' ' || c == '-' || c == '_') {
            if (slug.length() > 0 && slug[slug.length() - 1] != '_') {
                slug += '_';
            }
        }
    }
    // Remove trailing underscore
    if (slug.endsWith("_")) slug.remove(slug.length() - 1);
    
    // Fallback if empty
    if (slug.length() == 0) slug = String(millis());
    return slug;
}

// Utility: CORS Headers
void addCorsHeaders(AsyncWebServerResponse *response) {
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
}

void setup() {
    Serial.begin(115200);
    Serial.println("\nBooting ESP32_GroceryTruck...");

    if (!LittleFS.begin(true)) {
        Serial.println("An error has occurred while mounting LittleFS");
        return;
    }

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
    WiFi.softAP(AP_SSID, AP_PASSWORD);

    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

    // OPTIONS (CORS preflight)
    server.onNotFound([](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_OPTIONS) {
            request->send(200);
        } else {
            // Serve static files
            if (LittleFS.exists(request->url())) {
                AsyncWebServerResponse *response = request->beginResponse(LittleFS, request->url(), String(), false);
                response->addHeader("Cache-Control", "max-age=3600");
                request->send(response);
            } else if (request->url() == "/") {
                request->send(LittleFS, "/index.html", "text/html");
            } else if (request->url() == "/admin") {
                request->send(LittleFS, "/admin.html", "text/html");
            } else {
                request->send(404, "text/plain", "Not found");
            }
        }
    });

    // --- PRODUCTS API ---
    server.on("/api/products", HTTP_GET, [](AsyncWebServerRequest *request) {
        File file = LittleFS.open("/products.json", "r");
        if (!file) {
            request->send(200, "application/json", "[]");
            return;
        }
        
        JsonDocument doc;
        deserializeJson(doc, file);
        file.close();
        
        // Filter only active for public API
        JsonDocument activeDoc;
        JsonArray activeArray = activeDoc.to<JsonArray>();
        JsonArray array = doc.as<JsonArray>();
        
        for (JsonObject obj : array) {
            if (obj["active"] == true || !obj.containsKey("active")) {
                activeArray.add(obj);
            }
        }
        
        String responseStr;
        serializeJson(activeArray, responseStr);
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", responseStr);
        addCorsHeaders(response);
        request->send(response);
    });

    server.on("/api/products/all", HTTP_GET, [](AsyncWebServerRequest *request) {
        File file = LittleFS.open("/products.json", "r");
        if (!file) {
            request->send(200, "application/json", "[]");
            return;
        }
        AsyncWebServerResponse *response = request->beginResponse(file, "/products.json", "application/json");
        addCorsHeaders(response);
        request->send(response);
        file.close();
    });

    server.on("/api/products/bulk", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->_tempObject) {
            request->send(400, "application/json", "{\"error\":\"No body\"}");
            return;
        }
        String* body = (String*)request->_tempObject;
        
        JsonDocument newProducts;
        DeserializationError error = deserializeJson(newProducts, *body);
        delete body; // Free memory
        request->_tempObject = NULL;

        if (error) {
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        // Load existing
        File file = LittleFS.open("/products.json", "r");
        JsonDocument db;
        if (file) {
            deserializeJson(db, file);
            file.close();
        }
        JsonArray dbArray = db.is<JsonArray>() ? db.as<JsonArray>() : db.to<JsonArray>();

        JsonArray arr = newProducts.as<JsonArray>();
        int imported = 0;
        
        for (JsonObject p : arr) {
            String name = p["name"] | "Produto";
            String slug = generateSlug(name);
            p["slug"] = slug;
            p["id"] = String(millis()) + String(imported);
            p["image"] = "/img/" + slug + ".jpg"; // Default
            if (!p.containsKey("active")) p["active"] = true;
            
            dbArray.add(p);
            imported++;
        }

        file = LittleFS.open("/products.json", "w");
        serializeJson(db, file);
        file.close();

        String res = "{\"imported\":" + String(imported) + ",\"skipped\":0,\"errors\":[]}";
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", res);
        addCorsHeaders(response);
        request->send(response);
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (index == 0) {
            request->_tempObject = new String();
        }
        String* body = (String*)request->_tempObject;
        body->concat((const char*)data, len);
    });

    // --- IMAGES API ---
    server.on("/api/images", HTTP_GET, [](AsyncWebServerRequest *request) {
        File dir = LittleFS.open("/img");
        JsonDocument doc;
        JsonArray arr = doc["images"].to<JsonArray>();
        
        File file = dir.openNextFile();
        while(file) {
            arr.add(String(file.name()));
            file = dir.openNextFile();
        }
        
        String resStr;
        serializeJson(doc, resStr);
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", resStr);
        addCorsHeaders(response);
        request->send(response);
    });

    server.on("/api/images/check", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("slug")) {
            request->send(400, "application/json", "{\"error\":\"Missing slug\"}");
            return;
        }
        String slug = request->getParam("slug")->value();
        String pathJpg = "/img/" + slug + ".jpg";
        String pathPng = "/img/" + slug + ".png";
        
        String resStr;
        if (LittleFS.exists(pathJpg)) {
            resStr = "{\"exists\":true,\"path\":\"" + pathJpg + "\"}";
        } else if (LittleFS.exists(pathPng)) {
            resStr = "{\"exists\":true,\"path\":\"" + pathPng + "\"}";
        } else {
            resStr = "{\"exists\":false,\"slug\":\"" + slug + "\"}";
        }
        
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", resStr);
        addCorsHeaders(response);
        request->send(response);
    });

    server.on("/api/products/image/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"success\":true}");
    }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        String path = "/img/" + filename;
        if (!index) {
            uploadFile = LittleFS.open(path, "w");
        }
        if (uploadFile) {
            uploadFile.write(data, len);
        }
        if (final) {
            if (uploadFile) {
                uploadFile.close();
            }
        }
    });

    // --- SALES API ---
    server.on("/api/sales", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->_tempObject) {
            request->send(400, "application/json", "{\"error\":\"No body\"}");
            return;
        }
        String* body = (String*)request->_tempObject;
        
        JsonDocument newSale;
        DeserializationError error = deserializeJson(newSale, *body);
        delete body;
        request->_tempObject = NULL;

        if (error) {
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        newSale["id"] = String(millis());
        newSale["timestamp"] = time(NULL); // basic timestamp, might need actual RTC setup

        // Save sale
        File file = LittleFS.open("/sales.json", "r");
        JsonDocument db;
        if (file) {
            deserializeJson(db, file);
            file.close();
        }
        JsonArray dbArray = db.is<JsonArray>() ? db.as<JsonArray>() : db.to<JsonArray>();
        dbArray.add(newSale);

        file = LittleFS.open("/sales.json", "w");
        serializeJson(db, file);
        file.close();
        
        // Decrement stock in products.json
        File pFile = LittleFS.open("/products.json", "r");
        JsonDocument pDb;
        if (pFile) {
            deserializeJson(pDb, pFile);
            pFile.close();
            JsonArray pArray = pDb.as<JsonArray>();
            JsonArray items = newSale["items"];
            
            for (JsonObject item : items) {
                String pId = item["product_id"];
                int qty = item["qty"];
                for (JsonObject p : pArray) {
                    if (p["id"] == pId) {
                        int stock = p["stock"] | 0;
                        p["stock"] = max(0, stock - qty);
                        break;
                    }
                }
            }
            pFile = LittleFS.open("/products.json", "w");
            serializeJson(pDb, pFile);
            pFile.close();
        }

        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{\"success\":true}");
        addCorsHeaders(response);
        request->send(response);

    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (index == 0) request->_tempObject = new String();
        String* body = (String*)request->_tempObject;
        body->concat((const char*)data, len);
    });

    server.on("/api/sales", HTTP_GET, [](AsyncWebServerRequest *request) {
        File file = LittleFS.open("/sales.json", "r");
        if (!file) {
            request->send(200, "application/json", "[]");
            return;
        }
        AsyncWebServerResponse *response = request->beginResponse(file, "/sales.json", "application/json");
        addCorsHeaders(response);
        request->send(response);
        file.close();
    });

    server.on("/modelo", HTTP_GET, [](AsyncWebServerRequest *request) {
        if(LittleFS.exists("/produtos_modelo.csv")){
            AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/produtos_modelo.csv", "text/csv");
            response->addHeader("Content-Disposition", "attachment; filename=\"produtos_modelo.csv\"");
            request->send(response);
        } else {
            request->send(404, "text/plain", "Model not found");
        }
    });

    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    // Handled by Async Web Server
    delay(10);
}
