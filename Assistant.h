#pragma once
#pragma once

#include "types.h"
#include "MongoDBHelper.h"
#include "OllamaClient.h"
#include <string>
#include <vector>
#include <algorithm>

class Assistant {
public:
    explicit Assistant(
        const std::string& mongo_uri = "mongodb://localhost:27017",
        const std::string& ollama_url = "http://127.0.0.1:11434"
    );

    // Главный метод — получить ответ ассистента
    std::string getResponse(
        const std::string& user_query,
        const std::string& device_id
    );

private:
    MongoDBHelper mongo_;
    OllamaClient ollama_;

    // Константы моделей
    static constexpr const char* GEN_MODEL = "qwen3";
    static constexpr const char* EMBED_MODEL = "nomic-embed-text";

    // Системный промпт
    std::string buildSystemPrompt(const std::string& instrument_name) const;

    // Сборка RAG-контекста из топ-N секций
    std::string buildRagContext(
        const InstrumentData& instr,
        const std::vector<float>& query_embedding,
        int top_k = 3
    ) const;

    // Cosine similarity (вынесена сюда для удобства)
    static float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b);
};