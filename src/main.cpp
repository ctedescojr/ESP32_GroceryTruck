#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WiFi.h>

const char *AP_SSID     = "GroceryTruck";
const char *AP_PASSWORD = "fiorino123";
IPAddress   AP_IP(192, 168, 4, 1);
IPAddress   AP_GATEWAY(192, 168, 4, 1);
IPAddress   AP_SUBNET(255, 255, 255, 0);

AsyncWebServer server(80);
File           uploadFile;

// Utility: Normalize and Generate Slug
String generateSlug(String input) {
    String slug = "";
    input.toLowerCase();
    for (size_t i = 0; i < input.length(); i++) {
        char c = input[i];
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            slug += c;
        } else if (c == ' ' || c == '-' || c == '_') {
            if (slug.length() > 0 && slug[slug.length() - 1] != '_') {
                slug += '_';
            }
        }
    }
    if (slug.endsWith("_"))
        slug.remove(slug.length() - 1);
    if (slug.length() == 0)
        slug = String(millis());
    return slug;
}

// Utility: CORS Headers
void addCorsHeaders(AsyncWebServerResponse *response) {
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
}

class ProductUpdateHandler : public AsyncWebHandler {
  public:
    ProductUpdateHandler() {
    }
    bool canHandle(AsyncWebServerRequest *request) override {
        if (request->url().startsWith("/api/products/") && request->url().length() > 14) {
            if (request->url() == "/api/products/bulk" || request->url() == "/api/products/all" ||
                request->url() == "/api/products/image/upload")
                return false;
            return request->method() == HTTP_PUT || request->method() == HTTP_DELETE ||
                   request->method() == HTTP_OPTIONS;
        }
        return false;
    }
    void handleRequest(AsyncWebServerRequest *request) override {
        if (request->method() == HTTP_OPTIONS) {
            AsyncWebServerResponse *response = request->beginResponse(200);
            addCorsHeaders(response);
            request->send(response);
            return;
        }
        if (request->method() == HTTP_DELETE) {
            String       id   = request->url().substring(14);
            File         file = LittleFS.open("/products.json", "r");
            JsonDocument db;
            if (file) {
                deserializeJson(db, file);
                file.close();
            }
            JsonArray    arr   = db.as<JsonArray>();
            bool         found = false;
            JsonDocument newDb;
            JsonArray    newArr = newDb.to<JsonArray>();
            for (JsonObject p : arr) {
                if (p["id"] == id)
                    found = true;
                else
                    newArr.add(p);
            }
            file = LittleFS.open("/products.json", "w");
            serializeJson(newDb, file);
            file.close();

            AsyncWebServerResponse *response =
                request->beginResponse(200, "application/json",
                                       found ? "{\"success\":true}" : "{\"error\":\"not found\"}");
            addCorsHeaders(response);
            request->send(response);
            return;
        }
        if (request->method() == HTTP_PUT) {
            if (request->_tempObject) {
                String      *body = (String *)request->_tempObject;
                String       id   = request->url().substring(14);
                JsonDocument updateDoc;
                deserializeJson(updateDoc, *body);
                delete body;
                request->_tempObject = NULL;

                File         file = LittleFS.open("/products.json", "r");
                JsonDocument db;
                if (file) {
                    deserializeJson(db, file);
                    file.close();
                }
                JsonArray arr   = db.as<JsonArray>();
                bool      found = false;
                for (JsonObject p : arr) {
                    if (p["id"] == id) {
                        found = true;
                        for (JsonPair kv : updateDoc.as<JsonObject>()) {
                            if (kv.key() != "id")
                                p[kv.key()] = kv.value();
                        }
                        if (updateDoc["name"].is<JsonVariant>() &&
                            !updateDoc["slug"].is<JsonVariant>()) {
                            p["slug"] = generateSlug(updateDoc["name"].as<String>());
                        }
                        break;
                    }
                }
                file = LittleFS.open("/products.json", "w");
                serializeJson(db, file);
                file.close();

                AsyncWebServerResponse *response = request->beginResponse(
                    200, "application/json",
                    found ? "{\"success\":true}" : "{\"error\":\"not found\"}");
                addCorsHeaders(response);
                request->send(response);
            } else {
                request->send(400, "application/json", "{\"error\":\"No body\"}");
            }
        }
    }
    void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index,
                    size_t total) override {
        if (request->method() == HTTP_PUT) {
            if (index == 0)
                request->_tempObject = new String();
            String *body = (String *)request->_tempObject;
            body->concat((const char *)data, len);
        }
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("\nBooting ESP32_GroceryTruck v2...");

    if (!LittleFS.begin(true)) {
        Serial.println("An error has occurred while mounting LittleFS");
        return;
    }

    if (!LittleFS.exists("/img")) {
        LittleFS.mkdir("/img");
    }

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

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
                AsyncWebServerResponse *response =
                    request->beginResponse(LittleFS, request->url(), String(), false);
                response->addHeader("Cache-Control", "max-age=3600");
                request->send(response);
            } else {
                request->send(404, "text/plain", "Not found");
            }
        }
    });

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(204); });

    server.addHandler(new ProductUpdateHandler());

    server.on("/api/products", HTTP_GET, [](AsyncWebServerRequest *request) {
        File file = LittleFS.open("/products.json", "r");
        if (!file) {
            AsyncWebServerResponse *response =
                request->beginResponse(200, "application/json", "[]");
            addCorsHeaders(response);
            request->send(response);
            return;
        }
        JsonDocument doc;
        deserializeJson(doc, file);
        file.close();
        JsonDocument activeDoc;
        JsonArray    activeArray = activeDoc.to<JsonArray>();
        JsonArray    array       = doc.as<JsonArray>();
        for (JsonObject obj : array) {
            if (obj["active"] == true || !obj["active"].is<JsonVariant>())
                activeArray.add(obj);
        }
        String res;
        serializeJson(activeArray, res);
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", res);
        addCorsHeaders(response);
        request->send(response);
    });

    server.on("/api/products/all", HTTP_GET, [](AsyncWebServerRequest *request) {
        File file = LittleFS.open("/products.json", "r");
        if (!file) {
            AsyncWebServerResponse *response =
                request->beginResponse(200, "application/json", "[]");
            addCorsHeaders(response);
            request->send(response);
            return;
        }
        AsyncWebServerResponse *response =
            request->beginResponse(file, "/products.json", "application/json");
        addCorsHeaders(response);
        request->send(response);
        file.close();
    });

    server.on(
        "/api/products/bulk", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            if (request->_tempObject) {
                String              *body = (String *)request->_tempObject;
                JsonDocument         newProducts;
                DeserializationError error = deserializeJson(newProducts, *body);
                delete body;
                request->_tempObject = NULL;

                if (error) {
                    String errorMsg = "{\"error\":\"Invalid JSON: ";
                    errorMsg += error.c_str();
                    errorMsg += "\"}";
                    AsyncWebServerResponse *response =
                        request->beginResponse(400, "application/json", errorMsg);
                    addCorsHeaders(response);
                    request->send(response);
                    return;
                }

                File         file = LittleFS.open("/products.json", "r");
                JsonDocument db;
                if (file) {
                    deserializeJson(db, file);
                    file.close();
                }
                JsonArray dbArray  = db.is<JsonArray>() ? db.as<JsonArray>() : db.to<JsonArray>();
                JsonArray arr      = newProducts.as<JsonArray>();
                int       imported = 0;
                int       updated  = 0;

                for (JsonObject p : arr) {
                    String name = p["name"] | "Produto";
                    String slug = generateSlug(name);

                    // Check if product with this slug already exists
                    JsonObject existing;
                    bool       found = false;
                    for (JsonObject item : dbArray) {
                        if (item["slug"] == slug) {
                            existing = item;
                            found    = true;
                            break;
                        }
                    }

                    if (found) {
                        // Update existing
                        if (p["category"].is<JsonVariant>())
                            existing["category"] = p["category"];
                        if (p["description"].is<JsonVariant>())
                            existing["description"] = p["description"];
                        if (p["price_cost"].is<JsonVariant>())
                            existing["price_cost"] = p["price_cost"];
                        if (p["price_sell"].is<JsonVariant>())
                            existing["price_sell"] = p["price_sell"];
                        if (p["stock"].is<JsonVariant>())
                            existing["stock"] = p["stock"];
                        if (p["active"].is<JsonVariant>())
                            existing["active"] = p["active"];
                        updated++;
                    } else {
                        // Add new
                        p["slug"] = slug;
                        p["id"]   = String(millis()) + String(imported);
                        if (!p["image"].is<JsonVariant>()) {
                            String path = "/img/" + slug + ".jpg";
                            if (!LittleFS.exists(path)) {
                                String pngPath = "/img/" + slug + ".png";
                                if (LittleFS.exists(pngPath))
                                    path = pngPath;
                                else
                                    path = "";
                            }
                            p["image"] = path;
                        }
                        if (!p["active"].is<JsonVariant>())
                            p["active"] = true;
                        dbArray.add(p);
                        imported++;
                    }
                }
                file = LittleFS.open("/products.json", "w");
                serializeJson(db, file);
                file.close();

                JsonDocument resDoc;
                resDoc["imported"] = imported;
                resDoc["updated"]  = updated;
                resDoc["skipped"]  = 0;
                resDoc["errors"].to<JsonArray>();

                String res;
                serializeJson(resDoc, res);

                AsyncWebServerResponse *response =
                    request->beginResponse(200, "application/json", res);
                addCorsHeaders(response);
                request->send(response);
            } else {
                request->send(400, "application/json", "{\"error\":\"No body\"}");
            }
        },
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                String *body = new String();
                body->reserve(total);
                request->_tempObject = body;
            }
            String *body = (String *)request->_tempObject;
            body->concat((const char *)data, len);
        });

    server.on("/api/images", HTTP_GET, [](AsyncWebServerRequest *request) {
        File         dir = LittleFS.open("/img");
        JsonDocument doc;
        JsonArray    arr = doc["images"].to<JsonArray>();
        if (dir) {
            File file = dir.openNextFile();
            while (file) {
                arr.add(String(file.name()));
                file = dir.openNextFile();
            }
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
        String slug    = request->getParam("slug")->value();
        String pathJpg = "/img/" + slug + ".jpg";
        String pathPng = "/img/" + slug + ".png";
        String resStr;
        if (LittleFS.exists(pathJpg))
            resStr = "{\"exists\":true,\"path\":\"" + pathJpg + "\"}";
        else if (LittleFS.exists(pathPng))
            resStr = "{\"exists\":true,\"path\":\"" + pathPng + "\"}";
        else
            resStr = "{\"exists\":false,\"slug\":\"" + slug + "\"}";
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", resStr);
        addCorsHeaders(response);
        request->send(response);
    });

    server.on(
        "/api/products/image/upload", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            if (request->_tempObject) {
                String                 *msg  = (String *)request->_tempObject;
                int                     code = msg->indexOf("error") > 0 ? 413 : 200;
                AsyncWebServerResponse *response =
                    request->beginResponse(code, "application/json", *msg);
                addCorsHeaders(response);
                request->send(response);
                delete msg;
                request->_tempObject = NULL;
            } else {
                AsyncWebServerResponse *response = request->beginResponse(
                    400, "application/json", "{\"error\":\"Upload failed\"}");
                addCorsHeaders(response);
                request->send(response);
            }
        },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len,
           bool final) {
            String path = "/img/" + filename;
            if (!index) {
                request->_tempObject = new String();
                if (request->contentLength() > 102400) {
                    *((String *)request->_tempObject) =
                        "{\"error\":\"too_large\",\"max_bytes\":102400}";
                    return;
                }
                uploadFile = LittleFS.open(path, "w");
            }
            String *msg = (String *)request->_tempObject;
            if (msg && msg->indexOf("error") > 0)
                return;
            if (index + len > 102400) {
                if (uploadFile) {
                    uploadFile.close();
                    uploadFile = File();
                }
                LittleFS.remove(path);
                *msg = "{\"error\":\"too_large\",\"max_bytes\":102400}";
                return;
            }
            if (uploadFile)
                uploadFile.write(data, len);
            if (final) {
                if (uploadFile)
                    uploadFile.close();
                if (msg->length() == 0) {
                    File   f    = LittleFS.open(path, "r");
                    size_t size = f ? f.size() : 0;
                    if (f)
                        f.close();
                    *msg = "{\"saved\":\"" + path + "\",\"size_bytes\":" + String(size) + "}";

                    // Auto-heal products.json extension
                    String slug   = filename;
                    int    dotIdx = slug.lastIndexOf('.');
                    if (dotIdx > 0)
                        slug = slug.substring(0, dotIdx);

                    File pFile = LittleFS.open("/products.json", "r");
                    if (pFile) {
                        JsonDocument db;
                        deserializeJson(db, pFile);
                        pFile.close();
                        JsonArray arr     = db.as<JsonArray>();
                        bool      changed = false;
                        for (JsonObject p : arr) {
                            if (p["slug"] == slug) {
                                p["image"] = path;
                                changed    = true;
                            }
                        }
                        if (changed) {
                            pFile = LittleFS.open("/products.json", "w");
                            serializeJson(db, pFile);
                            pFile.close();
                        }
                    }
                }
            }
        });

    server.on(
        "/api/products", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            if (request->_tempObject) {
                String      *body = (String *)request->_tempObject;
                JsonDocument newProduct;
                deserializeJson(newProduct, *body);
                delete body;
                request->_tempObject = NULL;

                String name        = newProduct["name"] | "Produto";
                String slug        = generateSlug(name);
                newProduct["slug"] = slug;
                newProduct["id"]   = String(millis());
                if (!newProduct["image"].is<JsonVariant>()) {
                    String path = "/img/" + slug + ".jpg";
                    if (!LittleFS.exists(path)) {
                        String pngPath = "/img/" + slug + ".png";
                        if (LittleFS.exists(pngPath))
                            path = pngPath;
                        else
                            path = ""; // LEAVE EMPTY if no file exists yet
                    }
                    newProduct["image"] = path;
                }
                if (!newProduct["active"].is<JsonVariant>())
                    newProduct["active"] = true;

                File         file = LittleFS.open("/products.json", "r");
                JsonDocument db;
                if (file) {
                    deserializeJson(db, file);
                    file.close();
                }
                JsonArray dbArray = db.is<JsonArray>() ? db.as<JsonArray>() : db.to<JsonArray>();
                dbArray.add(newProduct);
                file = LittleFS.open("/products.json", "w");
                serializeJson(db, file);
                file.close();

                String res;
                serializeJson(newProduct, res);
                AsyncWebServerResponse *response =
                    request->beginResponse(200, "application/json", res);
                addCorsHeaders(response);
                request->send(response);
            } else {
                request->send(400, "application/json", "{\"error\":\"No body\"}");
            }
        },
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index == 0)
                request->_tempObject = new String();
            String *body = (String *)request->_tempObject;
            body->concat((const char *)data, len);
        });

    server.on(
        "/api/sales", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            if (request->_tempObject) {
                String              *body = (String *)request->_tempObject;
                JsonDocument         newSale;
                DeserializationError error = deserializeJson(newSale, *body);
                delete body;
                request->_tempObject = NULL;
                if (error) {
                    AsyncWebServerResponse *response = request->beginResponse(
                        400, "application/json", "{\"error\":\"Invalid JSON\"}");
                    addCorsHeaders(response);
                    request->send(response);
                    return;
                }

                newSale["id"] = String(millis());
                if (!newSale["timestamp"].is<JsonVariant>())
                    newSale["timestamp"] = time(NULL);
                if (!newSale["date"].is<JsonVariant>())
                    newSale["date"] = "1970-01-01"; // Fallback if client doesn't send date

                File         file = LittleFS.open("/sales.json", "r");
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

                File         pFile = LittleFS.open("/products.json", "r");
                JsonDocument pDb;
                if (pFile) {
                    deserializeJson(pDb, pFile);
                    pFile.close();
                    JsonArray pArray = pDb.as<JsonArray>();
                    JsonArray items  = newSale["items"];
                    for (JsonObject item : items) {
                        String pId = item["product_id"];
                        int    qty = item["qty"];
                        for (JsonObject p : pArray) {
                            if (p["id"] == pId) {
                                int stock  = p["stock"] | 0;
                                p["stock"] = max(0, stock - qty);
                                break;
                            }
                        }
                    }
                    pFile = LittleFS.open("/products.json", "w");
                    serializeJson(pDb, pFile);
                    pFile.close();
                }
                AsyncWebServerResponse *response =
                    request->beginResponse(200, "application/json", "{\"success\":true}");
                addCorsHeaders(response);
                request->send(response);
            } else {
                request->send(400, "application/json", "{\"error\":\"No body\"}");
            }
        },
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index == 0)
                request->_tempObject = new String();
            String *body = (String *)request->_tempObject;
            body->concat((const char *)data, len);
        });

    server.on("/api/sales", HTTP_GET, [](AsyncWebServerRequest *request) {
        File file = LittleFS.open("/sales.json", "r");
        if (!file) {
            AsyncWebServerResponse *response =
                request->beginResponse(200, "application/json", "[]");
            addCorsHeaders(response);
            request->send(response);
            return;
        }
        JsonDocument db;
        deserializeJson(db, file);
        file.close();
        String filterDate = request->hasParam("date") ? request->getParam("date")->value() : "";
        if (filterDate == "") {
            String res;
            serializeJson(db, res);
            AsyncWebServerResponse *response = request->beginResponse(200, "application/json", res);
            addCorsHeaders(response);
            request->send(response);
        } else {
            JsonDocument filtered;
            JsonArray    fArr = filtered.to<JsonArray>();
            JsonArray    arr  = db.as<JsonArray>();
            for (JsonObject s : arr) {
                if (s["date"] == filterDate)
                    fArr.add(s);
            }
            String res;
            serializeJson(fArr, res);
            AsyncWebServerResponse *response = request->beginResponse(200, "application/json", res);
            addCorsHeaders(response);
            request->send(response);
        }
    });

    server.on("/api/sales/summary", HTTP_GET, [](AsyncWebServerRequest *request) {
        File         file = LittleFS.open("/sales.json", "r");
        JsonDocument db;
        if (file) {
            deserializeJson(db, file);
            file.close();
        }
        JsonArray arr        = db.as<JsonArray>();
        String    filterDate = request->hasParam("date") ? request->getParam("date")->value() : "";
        float     totalRevenue = 0;
        int       totalSales   = 0;
        JsonDocument summary;
        summary["total_revenue"] = 0;
        summary["total_sales"]   = 0;
        JsonObject itemsSold     = summary["items_sold"].to<JsonObject>();

        for (JsonObject s : arr) {
            String sDate = s["date"] | "";
            if (filterDate == "" || sDate == filterDate) {
                totalSales++;
                totalRevenue += s["total"].as<float>();
                JsonArray items = s["items"];
                for (JsonObject item : items) {
                    String pName = item["name"];
                    int    qty   = item["qty"];
                    if (itemsSold[pName].is<JsonVariant>())
                        itemsSold[pName] = itemsSold[pName].as<int>() + qty;
                    else
                        itemsSold[pName] = qty;
                }
            }
        }
        summary["total_revenue"] = totalRevenue;
        summary["total_sales"]   = totalSales;
        String res;
        serializeJson(summary, res);
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", res);
        addCorsHeaders(response);
        request->send(response);
    });

    server.on("/api/system/storage", HTTP_GET, [](AsyncWebServerRequest *request) {
        size_t total = LittleFS.totalBytes();
        size_t used  = LittleFS.usedBytes();
        String res   = "{\"total_bytes\":" + String(total) + ",\"used_bytes\":" + String(used) +
                       ",\"free_bytes\":" + String(total - used) + "}";
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", res);
        addCorsHeaders(response);
        request->send(response);
    });

    server.on("/modelo", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/produtos_modelo.csv")) {
            AsyncWebServerResponse *response =
                request->beginResponse(LittleFS, "/produtos_modelo.csv", "text/csv");
            response->addHeader("Content-Disposition",
                                "attachment; filename=\"produtos_modelo.csv\"");
            request->send(response);
        } else {
            request->send(404, "text/plain", "Model not found");
        }
    });

    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    delay(10);
}
