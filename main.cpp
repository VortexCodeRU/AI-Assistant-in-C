// main.cpp
// Простая версия с выбором модели и одним запросом
// Оптимизировано под Windows-1251

#include "Assistant.h"
#include <iostream>
#include <string>
#include <windows.h>

// Функция конвертации UTF-8 -> Windows-1251 (самый надёжный вариант для русской консоли)
std::string utf8_to_cp1251(const std::string& utf8)
{
    if (utf8.empty()) return utf8;

    // UTF-8 -> UTF-16
    int wide_len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (wide_len <= 0) return utf8;

    std::wstring wide(wide_len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], wide_len);

    // UTF-16 -> CP1251
    int cp_len = WideCharToMultiByte(1251, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (cp_len <= 0) return utf8;

    std::string cp1251(cp_len, '\0');
    WideCharToMultiByte(1251, 0, wide.c_str(), -1, &cp1251[0], cp_len, nullptr, nullptr);

    if (!cp1251.empty() && cp1251.back() == '\0')
        cp1251.pop_back();

    return cp1251;
}

void print_ru(const std::string& text)
{
    std::cout << utf8_to_cp1251(text) << std::endl;
}

int main()
{
    // Настраиваем консоль на CP1251 (самый стабильный вариант для русского текста)
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);

    print_ru("=== Signal-Master AI Assistant ===");
    print_ru("");

    // Выбор модели
    int choice = 0;
    print_ru("Выберите модель нейросети:");
    print_ru("1 - Быстрая (qwen3:1.7b)");
    print_ru("2 - Умная (qwen3:4b)");
    std::cout << "Ваш выбор: ";
    std::cin >> choice;
    std::cin.ignore(); // очищаем буфер ввода

    std::string model_name;
    if (choice == 2) {
        model_name = Assistant::DEEP_MODEL;
        print_ru("Выбрана умная модель: qwen3:4b");
    }
    else {
        model_name = Assistant::FAST_MODEL;
        print_ru("Выбрана быстрая модель: qwen3:1.7b");
    }

    // Создаём ассистента с выбранной моделью
    Assistant assistant("mongodb://localhost:27017",
        "http://127.0.0.1:11434",
        model_name);

    // Ввод идентификатора прибора
    std::string device_id;
    print_ru("\nВведите модель прибора (например: DG2052): ");
    std::getline(std::cin, device_id);

    if (device_id.empty()) {
        print_ru("Идентификатор прибора не введён.");
        return 0;
    }

    print_ru("Подключён прибор: " + device_id);
    print_ru("----------------------------------------");

    // Ввод запроса пользователя
    std::string user_query;
    print_ru("Введите ваш запрос:");
    std::getline(std::cin, user_query);

    if (user_query.empty()) {
        print_ru("Запрос не введён.");
        return 0;
    }

    // Получаем ответ
    print_ru("\nОбрабатываю запрос...");
    std::string answer = assistant.getResponse(user_query, device_id);

    // Вывод ответа
    print_ru("\nОтвет ассистента:");
    print_ru("----------------------------------------");
    print_ru(answer);
    print_ru("----------------------------------------");

    print_ru("\nНажмите Enter для завершения...");
    std::cin.get();

    return 0;
}