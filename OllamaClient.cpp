// Общение с нейросетью Ollama (отправка / получение запросов)

#include "OllamaClient.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <windows.h>

using json = nlohmann::json;

// Вспомогательная функция для конвертации русской строки в UTF-8
std::string toUtf8(const std::string& str) {
    if (str.empty()) return str;

    // Получаем размер буфера для широких символов
    int wideSize = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
    if (wideSize <= 0) return str;

    // Конвертируем в широкие символы
    std::wstring wideStr(wideSize, L'\0');
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wideStr[0], wideSize);

    // Получаем размер буфера для UTF-8
    int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8Size <= 0) return str;

    // Конвертируем в UTF-8
    std::string utf8Str(utf8Size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8Str[0], utf8Size, nullptr, nullptr);

    // Убираем нулевой символ в конце
    if (!utf8Str.empty() && utf8Str.back() == '\0') {
        utf8Str.pop_back();
    }

    return utf8Str;
}

OllamaClient::OllamaClient(const std::string& base_url)
    : base_url_(base_url) {}

OllamaResponse OllamaClient::generateChat(
    const std::string& model,
    const std::string& system_prompt,
    const std::string& user_message,
    float temperature,
    int max_tokens,
    bool stream)
{
    OllamaResponse result;

    httplib::Client cli(base_url_);
    cli.set_connection_timeout(30);
    cli.set_default_headers({ {"Content-Type", "application/json; charset=utf-8"} });

    // Конвертируем все строки в UTF-8
    std::string model_utf8 = toUtf8(model);
    std::string system_prompt_utf8 = toUtf8(system_prompt);
    std::string user_message_utf8 = toUtf8(user_message);

    // Создаем JSON объект
    json request;
    request["model"] = model_utf8;

    // Создаем массив сообщений
    json messages = json::array();

    json system_msg;
    system_msg["role"] = "system";
    system_msg["content"] = system_prompt_utf8;
    messages.push_back(system_msg);

    json user_msg;
    user_msg["role"] = "user";
    user_msg["content"] = user_message_utf8;
    messages.push_back(user_msg);

    request["messages"] = messages;
    request["stream"] = stream;
    request["temperature"] = temperature;
    request["max_tokens"] = max_tokens;

    // Дампим с обработкой ошибок
    std::string body = request.dump();

    auto res = cli.Post("/api/chat", body, "application/json; charset=utf-8");

    if (!res) {
        result.error = "Нет ответа от сервера Ollama. Убедитесь, что Ollama запущена.";
        result.success = false;
        return result;
    }

    if (res->status != 200) {
        result.error = "HTTP ошибка " + std::to_string(res->status) + ": " + res->body;
        result.success = false;
        return result;
    }

    try {
        auto j = json::parse(res->body);

        if (j.contains("message") && j["message"].contains("content")) {
            result.content = j["message"]["content"].get<std::string>();
            result.success = true;
        }
        else {
            result.error = "Не найден ключ 'message.content' в ответе Ollama";
            result.success = false;
        }
    }
    catch (const std::exception& e) {
        result.error = "Ошибка парсинга JSON: " + std::string(e.what());
        result.success = false;
    }

    return result;
}

OllamaEmbedding OllamaClient::generateEmbedding(
    const std::string& embed_model,
    const std::string& text)
{
    OllamaEmbedding result;

    httplib::Client cli(base_url_);
    cli.set_connection_timeout(15);
    cli.set_default_headers({ {"Content-Type", "application/json; charset=utf-8"} });

    // Конвертируем строки в UTF-8
    std::string model_utf8 = toUtf8(embed_model);
    std::string text_utf8 = toUtf8(text);

    // Создаем JSON объект
    json request;
    request["model"] = model_utf8;
    request["input"] = text_utf8;

    std::string body = request.dump();

    auto res = cli.Post("/api/embed", body, "application/json; charset=utf-8");

    if (!res) {
        result.error = "Нет ответа от сервера Ollama. Убедитесь, что Ollama запущена.";
        result.success = false;
        return result;
    }

    if (res->status != 200) {
        result.error = "HTTP ошибка при получении эмбеддинга: " + std::to_string(res->status) + "\n" + res->body;
        result.success = false;
        return result;
    }

    try {
        auto j = json::parse(res->body);

        if (j.contains("embedding") && j["embedding"].is_array()) {
            result.vector = j["embedding"].get<std::vector<float>>();
            result.success = true;
        }
        else if (j.contains("embeddings") && j["embeddings"].is_array() && !j["embeddings"].empty()) {
            result.vector = j["embeddings"][0].get<std::vector<float>>();
            result.success = true;
        }
        else {
            result.error = "Не найден ключ 'embedding' или 'embeddings' в ответе";
            result.success = false;
        }
    }
    catch (const std::exception& e) {
        result.error = "Ошибка парсинга JSON эмбеддинга: " + std::string(e.what());
        result.success = false;
    }

    return result;
}