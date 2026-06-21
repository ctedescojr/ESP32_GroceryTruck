# ESP32 Grocery Truck POS 🚛

[🇺🇸 Read in English](#english) | [🇧🇷 Ler em Português](#portuguese)

---

<a id="english"></a>
## 🇺🇸 English

### Pro Bono Initiative 🤝
This project is a **100% Pro Bono initiative** developed completely free of charge to empower mobile merchants, food truck owners, and street vendors. The goal is to provide a robust, modern, and cost-free digital infrastructure that does not rely on internet connectivity or expensive monthly subscriptions.

### Overview
**ESP32 Grocery Truck** is a complete, offline Point of Sale (POS) and Inventory Management system embedded directly into an ESP32 microcontroller. 

It acts as its own Wi-Fi Access Point and Web Server. Both the business owner (admin) and the customers can connect directly to the ESP32's Wi-Fi network and access the storefront or management panel via any smartphone browser—**no internet connection required.**

### Key Features
*   **📡 100% Offline (Access Point Mode):** The ESP32 broadcasts its own Wi-Fi network. Everything is processed locally.
*   **🛒 Customer Storefront:** A mobile-first, user-friendly digital catalog where customers can browse products by category, view prices, and add items to a digital shopping cart.
*   **🔐 Admin Panel:** A protected dashboard for the merchant to manage their business.
*   **📦 Inventory Management:** Create, edit, and delete products. Automatically tracks stock levels and calculates profit margins.
*   **📊 Sales Reports:** Tracks daily revenue, total items sold, and highlights top-selling products.
*   **📑 Excel/CSV Bulk Import:** Upload a `.xlsx` or `.csv` file directly from your phone or PC to update your entire inventory at once. Parsed locally on the browser using an offline version of SheetJS.
*   **🖼️ Smart Image Uploads:** Upload product photos directly to the ESP32. The system automatically associates the image with the product and handles both `.jpg` and `.png` formats seamlessly.
*   **📺 Smart Display (CYD):** Optional support for CYD (Cheap Yellow Display) boards to show network status, connected clients, daily sales, and recent transaction details directly on the device's touch screen.

### Technical Stack
*   **Hardware:** ESP32 (4MB Flash minimum). *Optional: CYD (Cheap Yellow Display) board with ILI9341 touch screen.*
*   **Firmware:** C++ / Arduino Framework (PlatformIO).
*   **Web Server:** `ESPAsyncWebServer` & `AsyncTCP`.
*   **Storage:** LittleFS for static files and JSON databases (`products.json`, `sales.json`).
*   **Frontend:** Vanilla HTML, CSS, and JavaScript. Zero external CDNs.
*   **Partitioning:** Custom partition table allocating 2.4MB for the filesystem to accommodate the frontend, SheetJS, and product images.

### Installation (PlatformIO)
1. Clone this repository.
2. Build the firmware (choose your environment):
   * Without display: `pio run -e grocery_truck`
   * With CYD display: `pio run -e grocery_truck_cyd`
3. Upload the firmware to the ESP32: `pio run -e <environment> --target upload`
4. Upload the web files to LittleFS: `pio run -e <environment> --target uploadfs`
5. Connect to the Wi-Fi network `GroceryTruck` (Password: `fiorino123`).
6. Access the store at `http://192.168.4.1` or the admin panel at `http://192.168.4.1/admin`.

---

<a id="portuguese"></a>
## 🇧🇷 Português

### Iniciativa Pro Bono 🤝
Este projeto é uma **iniciativa 100% Pro Bono**, desenvolvida de forma totalmente gratuita para empoderar comerciantes itinerantes, donos de food trucks, feirantes e vendedores autônomos. O objetivo é fornecer uma infraestrutura digital robusta, moderna e sem custos, que não dependa de conexão com a internet ou assinaturas mensais caras.

### Visão Geral
O **ESP32 Grocery Truck** é um sistema completo e offline de Ponto de Venda (PDV) e Gerenciamento de Estoque, embutido diretamente em um microcontrolador ESP32.

Ele atua como seu próprio Roteador Wi-Fi (Access Point) e Servidor Web. Tanto o dono do negócio (admin) quanto os clientes podem se conectar diretamente ao Wi-Fi do ESP32 e acessar a loja ou o painel de gestão através de qualquer navegador de celular — **sem precisar de internet.**

### Principais Funcionalidades
*   **📡 100% Offline (Modo Access Point):** O ESP32 cria sua própria rede Wi-Fi. Tudo é processado localmente.
*   **🛒 Loja para o Cliente:** Um catálogo digital amigável, pensado para celulares (mobile-first), onde os clientes podem navegar por categorias, ver preços e adicionar itens a um carrinho de compras digital.
*   **🔐 Painel Administrativo:** Um ambiente protegido para o comerciante gerenciar o negócio.
*   **📦 Gestão de Estoque:** Crie, edite e exclua produtos. O sistema controla o estoque automaticamente após cada venda e calcula margens de lucro.
*   **📊 Relatórios de Vendas:** Acompanhe o faturamento diário, total de vendas e saiba quais são os produtos mais vendidos.
*   **📑 Importação em Massa (Excel/CSV):** Faça upload de um arquivo `.xlsx` ou `.csv` direto do celular ou PC para atualizar todo o seu estoque de uma vez. O processamento é feito no próprio navegador (SheetJS offline).
*   **🖼️ Upload Inteligente de Imagens:** Envie fotos dos produtos direto para a memória do ESP32. O sistema associa a imagem ao produto automaticamente e lida com formatos `.jpg` e `.png` sem problemas (compressão recomendada < 100KB).
*   **📺 Display Inteligente (CYD):** Suporte opcional para placas CYD (Cheap Yellow Display) para exibir status da rede, clientes conectados, vendas diárias e detalhes da última transação diretamente na tela touch do dispositivo.

### Stack Tecnológica
*   **Hardware:** ESP32 (Mínimo de 4MB Flash). *Opcional: Placa CYD (Cheap Yellow Display) com tela touch ILI9341.*
*   **Firmware:** C++ / Arduino Framework (PlatformIO).
*   **Servidor Web:** `ESPAsyncWebServer` & `AsyncTCP`.
*   **Armazenamento:** LittleFS para arquivos estáticos e banco de dados JSON (`products.json`, `sales.json`).
*   **Frontend:** HTML, CSS e JavaScript puros (Vanilla). Zero dependência de internet/CDNs.
*   **Particionamento:** Tabela de partições customizada reservando 2.4MB para o sistema de arquivos, garantindo espaço para o site, biblioteca SheetJS e fotos dos produtos.

### Instalação (PlatformIO)
1. Clone este repositório.
2. Compile o firmware (escolha seu ambiente):
   * Sem display: `pio run -e grocery_truck`
   * Com display CYD: `pio run -e grocery_truck_cyd`
3. Grave o firmware no ESP32: `pio run -e <ambiente> --target upload`
4. Grave os arquivos do site na memória LittleFS: `pio run -e <ambiente> --target uploadfs`
5. Conecte-se à rede Wi-Fi `GroceryTruck` (Senha: `fiorino123`).
6. Acesse a loja em `http://192.168.4.1` ou o painel em `http://192.168.4.1/admin`.