/**
 * @file display_manager.cpp
 * @brief Implementação do módulo de display para o CYD (ILI9341 240x320).
 *
 * Layout das telas:
 *
 *  BOOT        STATUS          SALES           LAST_SALE
 * ┌────────┐  ┌────────┐      ┌────────┐      ┌────────┐
 * │ LOGO   │  │ WiFi AP│      │ VENDAS │      │ ÚLTIMA │
 * │Grocery │  │GroceryT│      │  hoje  │      │ VENDA  │
 * │ Truck  │  │────────│      │────────│      │────────│
 * │        │  │IP:     │      │R$000.00│      │item x1 │
 * │ v2.0   │  │192.168.│      │        │      │item x2 │
 * │        │  │  4.1   │      │ 12     │      │────────│
 * │Iniciand│  │────────│      │ vendas │      │TOTAL   │
 * │   ...  │  │Clientes│      │        │      │R$00.00 │
 * │        │  │   3    │      │        │      │        │
 * └────────┘  └────────┘      └────────┘      └────────┘
 */

#ifdef ENABLE_DISPLAY

#include "display_manager.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <TFT_eSPI.h>
#include <WiFi.h>

// ─── Constantes de cor (RGB565) ──────────────────────────────────────────────
static const uint16_t COLOR_BG      = TFT_BLACK;
static const uint16_t COLOR_PRIMARY = 0xD2A0; // Laranja #d35400 convertido para RGB565
static const uint16_t COLOR_WHITE   = TFT_WHITE;
static const uint16_t COLOR_GRAY    = 0x8410;
static const uint16_t COLOR_GREEN   = TFT_GREEN;
static const uint16_t COLOR_YELLOW  = TFT_YELLOW;

// ─── Configuração do display ─────────────────────────────────────────────────
// O CYD opera melhor em modo retrato (240 wide x 320 tall).
// Rotação 0 = retrato padrão com conector USB embaixo.
static const uint8_t DISPLAY_ROTATION = 0;

// Intervalo de rotação automática entre telas (ms)
static const unsigned long SCREEN_ROTATION_INTERVAL = 8000;

// ─── Estado interno ──────────────────────────────────────────────────────────
static TFT_eSPI tft = TFT_eSPI();

static DisplayScreen currentScreen    = DisplayScreen::BOOT;
static unsigned long lastScreenChange = 0;
static bool          initialized      = false;

// Cache da última venda para a tela LAST_SALE
static float  lastSaleTotal = 0.0f;
static String lastSaleItems = "";
static bool   hasLastSale   = false;

// ─── Funções auxiliares ───────────────────────────────────────────────────────

static void drawHeader(const String &title) {
    // Barra de topo com cor primária
    tft.fillRect(0, 0, 240, 36, COLOR_PRIMARY);
    tft.setTextColor(COLOR_WHITE, COLOR_PRIMARY);
    tft.setTextSize(2);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(title, 120, 18);
    tft.setTextDatum(TL_DATUM); // Reset para top-left
}

static void drawFooter(const String &text) {
    tft.fillRect(0, 300, 240, 20, COLOR_GRAY);
    tft.setTextColor(COLOR_WHITE, COLOR_GRAY);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(text, 120, 310);
    tft.setTextDatum(TL_DATUM);
}

static void drawDivider(int y) {
    tft.drawFastHLine(10, y, 220, COLOR_GRAY);
}

// Lê o total de vendas do dia e o número de vendas do sales.json
static bool getSalesToday(float &outTotal, int &outCount) {
    File file = LittleFS.open("/sales.json", "r");
    if (!file) {
        outTotal = 0;
        outCount = 0;
        return false;
    }

    // Pega a data atual baseado no timestamp do millis
    // (o ESP32 não tem RTC, usamos a data enviada pelo cliente na última venda)
    JsonDocument         db;
    DeserializationError err = deserializeJson(db, file);
    file.close();
    if (err)
        return false;

    JsonArray arr = db.as<JsonArray>();
    outTotal      = 0;
    outCount      = 0;

    // Pega a data da última venda como referência do "hoje"
    String todayDate = "";
    for (int i = arr.size() - 1; i >= 0; i--) {
        String d = arr[i]["date"] | "";
        if (d.length() == 10) { // YYYY-MM-DD
            todayDate = d;
            break;
        }
    }
    if (todayDate == "")
        return false;

    for (JsonObject s : arr) {
        if (s["date"] == todayDate) {
            outTotal += s["total"].as<float>();
            outCount++;
        }
    }
    return true;
}

// ─── Renderizadores de tela ───────────────────────────────────────────────────

static void renderBoot() {
    tft.fillScreen(COLOR_BG);

    // Retângulo decorativo de topo
    tft.fillRect(0, 0, 240, 80, COLOR_PRIMARY);
    tft.setTextColor(COLOR_WHITE, COLOR_PRIMARY);

    tft.setTextSize(3);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Grocery", 120, 30);
    tft.drawString("Truck", 120, 58);

    // Separador
    tft.fillRect(0, 80, 240, 3, COLOR_WHITE);

    tft.setTextColor(COLOR_GRAY, COLOR_BG);
    tft.setTextSize(1);
    tft.drawString("Fiorino Artesanal v2", 120, 110);

    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    tft.setTextSize(2);
    tft.drawString("Iniciando...", 120, 160);

    // Barra de progresso animada (estática por ora)
    tft.drawRect(20, 200, 200, 16, COLOR_PRIMARY);
    tft.fillRect(22, 202, 100, 12, COLOR_PRIMARY);

    tft.setTextColor(COLOR_GRAY, COLOR_BG);
    tft.setTextSize(1);
    tft.drawString("Aguarde o sistema subir", 120, 230);

    tft.setTextDatum(TL_DATUM);
}

static void renderStatus() {
    tft.fillScreen(COLOR_BG);
    drawHeader("Status do Sistema");

    // IP do AP
    String ip = WiFi.softAPIP().toString();
    tft.setTextColor(COLOR_GRAY, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(15, 50);
    tft.print("Rede Wi-Fi");

    tft.setTextColor(COLOR_PRIMARY, COLOR_BG);
    tft.setTextSize(2);
    tft.setCursor(15, 65);
    tft.print("GroceryTruck");

    drawDivider(90);

    tft.setTextColor(COLOR_GRAY, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(15, 100);
    tft.print("Endereco IP");

    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    tft.setTextSize(2);
    tft.setCursor(15, 115);
    tft.print(ip);

    drawDivider(145);

    tft.setTextColor(COLOR_GRAY, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(15, 155);
    tft.print("Clientes conectados");

    uint8_t clients = WiFi.softAPgetStationNum();
    tft.setTextColor(clients > 0 ? COLOR_GREEN : COLOR_YELLOW, COLOR_BG);
    tft.setTextSize(4);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(String(clients), 120, 210);
    tft.setTextDatum(TL_DATUM);

    tft.setTextColor(COLOR_GRAY, COLOR_BG);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(clients == 1 ? "cliente" : "clientes", 120, 245);
    tft.setTextDatum(TL_DATUM);

    drawFooter("Admin: " + ip + "/admin");
}

static void renderSales() {
    tft.fillScreen(COLOR_BG);
    drawHeader("Vendas de Hoje");

    float total = 0;
    int   count = 0;
    bool  ok    = getSalesToday(total, count);

    if (!ok && count == 0) {
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.setTextSize(2);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("Sem vendas", 120, 160);
        tft.drawString("registradas hoje", 120, 190);
        tft.setTextDatum(TL_DATUM);
        drawFooter("Aguardando clientes...");
        return;
    }

    // Total em R$
    tft.setTextColor(COLOR_GRAY, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(15, 50);
    tft.print("Total arrecadado");

    // Formata com 2 casas decimais manualmente (sem sprintf)
    String totalStr = "R$ " + String((long)total) + ",";
    int    cents    = (int)((total - (long)total) * 100 + 0.5f);
    if (cents < 10)
        totalStr += "0";
    totalStr += String(cents);

    tft.setTextColor(COLOR_PRIMARY, COLOR_BG);
    tft.setTextSize(3);
    tft.setCursor(15, 65);
    tft.print(totalStr);

    drawDivider(115);

    // Número de vendas
    tft.setTextColor(COLOR_GRAY, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(15, 125);
    tft.print("Numero de vendas");

    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    tft.setTextSize(4);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(String(count), 120, 195);
    tft.setTextDatum(TL_DATUM);

    tft.setTextColor(COLOR_GRAY, COLOR_BG);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(count == 1 ? "venda realizada" : "vendas realizadas", 120, 240);
    tft.setTextDatum(TL_DATUM);

    drawFooter("Ticket medio: R$" + String(count > 0 ? total / count : 0, 2));
}

static void renderLastSale() {
    tft.fillScreen(COLOR_BG);
    drawHeader("Ultima Venda");

    if (!hasLastSale) {
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.setTextSize(2);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("Nenhuma venda", 120, 160);
        tft.drawString("ainda", 120, 190);
        tft.setTextDatum(TL_DATUM);
        return;
    }

    // Itens (string pré-formatada, ex: "Mel x2\nQueijo x1")
    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    tft.setTextSize(2);
    int y     = 50;
    int start = 0;
    // Itera pela string de itens separada por '\n'
    for (int i = 0; i <= (int)lastSaleItems.length(); i++) {
        if (i == (int)lastSaleItems.length() || lastSaleItems[i] == '\n') {
            String line = lastSaleItems.substring(start, i);
            if (line.length() > 0) {
                // Trunca se muito longo para o display
                if (line.length() > 16)
                    line = line.substring(0, 15) + "~";
                tft.setCursor(15, y);
                tft.print(line);
                y += 28;
                if (y > 240)
                    break; // Evita overflow
            }
            start = i + 1;
        }
    }

    drawDivider(y + 5);

    // Total da venda
    tft.setTextColor(COLOR_GRAY, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(15, y + 15);
    tft.print("Total");

    String totalStr = "R$ " + String(lastSaleTotal, 2);
    tft.setTextColor(COLOR_PRIMARY, COLOR_BG);
    tft.setTextSize(3);
    tft.setCursor(15, y + 28);
    tft.print(totalStr);

    drawFooter("Obrigado pela compra!");
}

// ─── API pública ─────────────────────────────────────────────────────────────

void initDisplay() {
    // Backlight do CYD está no IO21
    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH);

    tft.init();
    tft.setRotation(DISPLAY_ROTATION);
    tft.fillScreen(COLOR_BG);

    renderBoot();
    lastScreenChange = millis();
    initialized      = true;

    Serial.println("[Display] CYD inicializado (ILI9341 240x320)");
}

void updateDisplay() {
    if (!initialized)
        return;

    unsigned long now = millis();

    // Rotação automática de tela (exceto BOOT e LAST_SALE)
    if (currentScreen != DisplayScreen::BOOT && currentScreen != DisplayScreen::LAST_SALE &&
        (now - lastScreenChange >= SCREEN_ROTATION_INTERVAL)) {

        displayNextScreen();
        return;
    }

    // Sai do BOOT após 3 segundos
    if (currentScreen == DisplayScreen::BOOT && (now - lastScreenChange >= 3000)) {
        currentScreen    = DisplayScreen::STATUS;
        lastScreenChange = now;
        renderStatus();
        return;
    }

    // LAST_SALE mostra por 10 segundos, depois volta ao STATUS
    if (currentScreen == DisplayScreen::LAST_SALE && (now - lastScreenChange >= 10000)) {
        currentScreen    = DisplayScreen::STATUS;
        lastScreenChange = now;
        renderStatus();
        return;
    }
}

void displayNotifySale(float saleTotal, const String &itemsSummary) {
    if (!initialized)
        return;

    lastSaleTotal = saleTotal;
    lastSaleItems = itemsSummary;
    hasLastSale   = true;

    currentScreen    = DisplayScreen::LAST_SALE;
    lastScreenChange = millis();
    renderLastSale();
}

void displayNextScreen() {
    if (!initialized)
        return;

    // Cicla entre STATUS e SALES
    if (currentScreen == DisplayScreen::STATUS) {
        currentScreen = DisplayScreen::SALES;
        renderSales();
    } else {
        currentScreen = DisplayScreen::STATUS;
        renderStatus();
    }
    lastScreenChange = millis();
}

#endif // ENABLE_DISPLAY