// main.cpp
// Точка входа проекта — интерактивный ассистент
// Учитывает проблемы с кодировками Windows (UTF-8 в Windows-1251 / CP1251)

#include "Assistant.h"
#include <iostream>
#include <string>
#include <windows.h>
#include <algorithm>  // для transform
#include <cctype>     // для tolower

// перевод с UTF-8 в CP1251
std::string utf8_to_cp1251(const std::string& utf8_str)
{
    if (utf8_str.empty()) return utf8_str;

    int wide_size = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
    if (wide_size <= 0) return utf8_str;

    std::wstring wide_str(wide_size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &wide_str[0], wide_size);

    int cp1251_size = WideCharToMultiByte(1251, 0, wide_str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (cp1251_size <= 0) return utf8_str;

    std::string cp1251_str(cp1251_size, '\0');
    WideCharToMultiByte(1251, 0, wide_str.c_str(), -1, &cp1251_str[0], cp1251_size, nullptr, nullptr);

    if (!cp1251_str.empty() && cp1251_str.back() == '\0')
        cp1251_str.pop_back();

    return cp1251_str;
}

void print_ru(const std::string& text_utf8)
{
    std::cout << utf8_to_cp1251(text_utf8) << std::endl;
}

// Функция для приведения строки к нижнему регистру
std::string toLower(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return result;
}

// Проверка на команду выхода
bool isExitCommand(const std::string& input)
{
    std::string lower = toLower(input);
    return lower == "exit" || 
           lower == "quit" || 
           lower == "выход" || 
           lower == "q" ||
           lower == "пока" ||
           lower == "выйти";
}

int main()
{
    // Настройка кодировки консоли
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);

    print_ru("========================================");
    print_ru("=== AI Assistant ===");
    print_ru("========================================");
    print_ru("");
    print_ru("Интерактивный режим общения");
    print_ru("Для выхода введите: exit, quit, выход или q");
    print_ru("========================================");
    print_ru("");

    Assistant assistant;
    std::string device_id = "DS1074Z";
    
    print_ru("Подключено устройство: " + device_id);
    print_ru("");
    
    int query_count = 0;
    
    while (true) {
        // Вывод приглашения
        std::cout << std::endl;
        print_ru("────────────────────────────────────────");
        print_ru("Вопрос #" + std::to_string(++query_count) + ":");
        print_ru("────────────────────────────────────────");
        print_ru("Ваш запрос (или 'exit' для выхода):");
        
        std::string user_query;
        std::getline(std::cin, user_query);
        
        // Проверка на пустой ввод
        if (user_query.empty()) {
            print_ru("Введите вопрос или команду exit для выхода.");
            continue;
        }
        
        // Проверка на выход
        if (isExitCommand(user_query)) {
            print_ru("");
            print_ru("========================================");
            print_ru("До свидания! Рад был помочь!");
            print_ru("========================================");
            break;
        }
        
        // Проверка на специальные команды
        std::string lower_query = toLower(user_query);
        if (lower_query == "clear" || lower_query == "cls" || lower_query == "очистить") {
            system("cls");
            print_ru("=== Signal-Master AI Assistant ===");
            print_ru("");
            continue;
        }
        
        if (lower_query == "help" || lower_query == "справка" || lower_query == "помощь") {
            print_ru("");
            print_ru("Доступные команды:");
            print_ru("  exit, quit, выход, q - завершить работу");
            print_ru("  clear, cls, очистить - очистить экран");
            print_ru("  help, справка, помощь - показать эту справку");
            print_ru("");
            print_ru("Также вы можете задавать любые вопросы о приборе.");
            print_ru("Примеры вопросов:");
            print_ru("  - Как измерить напряжение?");
            print_ru("  - Что такое Auto Scale?");
            print_ru("  - Как настроить курсоры?");
            print_ru("  - Как очистить экран?");
            print_ru("");
            continue;
        }
        
        // Вывод информации о запросе
        print_ru("");
        print_ru("Обрабатываю запрос: \"" + user_query + "\"");
        print_ru("Ожидание ответа от нейросети...");
        print_ru("");
        
        // Получение ответа от ассистента
        std::string answer = assistant.getResponse(user_query, device_id);
        
        // Вывод ответа
        print_ru("════════════════════════════════════════");
        print_ru("Ответ ассистента:");
        print_ru("════════════════════════════════════════");
        print_ru(answer);
        print_ru("════════════════════════════════════════");
        
        // Добавляем разделитель для следующего вопроса
        print_ru("");
        print_ru("Для продолжения задайте следующий вопрос...");
    }
    
    return 0;
}