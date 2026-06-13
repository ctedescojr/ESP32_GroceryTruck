/**
 * @file utils.cpp
 * @brief Implementação das funções utilitárias.
 */

#include "utils.h"

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

void addCorsHeaders(AsyncWebServerResponse *response) {
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
}