// Хранение общих типов, структур и прочих данных.
// Здесь описано, как выглядит информация о приборе и ответ от Ollama.
#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <cstdint>

struct CommandInfo { // Описывает одну конкретную команду
    std::string scpi; // сама команда
    std::string description; // описание команды для user
    std::string group; // к какой группе принадлежит команда: math, channels и тд
};

struct Section {
    std::string title; // имя секции: "MATH Function", "Trigger Settings"
    std::string content; // её содержимое
    std::vector<float> embedding; // Числовой вектор этой секции, то есть эмбеддинг
};

struct InstrumentData { // При нахождении прибора в БД, все данные копируются сюда
    std::string device_id; // имя устройства, полученное *IDN?
    std::string normalized_name; // полное название прибора "Rigol DS1074Z"
    std::string manufacturer; // имя прибора
    std::string model; // модель прибора
    std::string short_description; // "4-канальный осциллограф 70 МГц"
    std::vector<Section> sections; // разбитое руководство
    std::map<std::string, CommandInfo> commands; // все команды

    bool is_valid() const { return !normalized_name.empty(); }
};

struct OllamaEmbedding { // Генерация эмбеддинга
    std::vector<float> vector;
    bool success = false; // проверка на успешность генерации
    std::string error; // сообщение ошибки при неправильной генерации
};

struct OllamaResponse { // Возвращаение ответа в чат
    std::string content;
    bool success = false;
    std::string error;
};