// Мозг проекта - отвечает за всё
// Главная логика AI-ассистента (берёт данные / составляет промпт / отправляет Ollama / получает ответ)
// *Промпт - запрос, который user даёт нейросети для выполнения определённой задачи

#include "Assistant.h"
#include <iostream>
#include <cmath>
#include <limits>

Assistant::Assistant(const std::string& mongo_uri, const std::string& ollama_url)
    : mongo_(mongo_uri),
    ollama_(ollama_url)
{
    MongoDBHelper::init();
    if (!mongo_.isConnected()) {
        std::cerr << "[Assistant] MongoDB не подключён!\n";
    }
}
// Главный алгоритм работы
std::string Assistant::getResponse(const std::string& user_query, const std::string& device_id)
{
    // 1. Находим прибор
    auto opt_instr = mongo_.findInstrument(device_id);
    if (!opt_instr) {
        return "Устройство не распознано. Пожалуйста, загрузите руководство пользователя в формате PDF или DOCX.";
    }

    const auto& instr = *opt_instr;

    // 2. Генерируем эмбеддинг запроса пользователя
    auto emb_res = ollama_.generateEmbedding(EMBED_MODEL, user_query);
    if (!emb_res.success) {
        return "Ошибка генерации эмбеддинга: " + emb_res.error;
    }

    const auto& query_emb = emb_res.vector;

    // 3. Собираем RAG-контекст из самых релевантных секций
    std::string rag_context = buildRagContext(instr, query_emb, 3);

    // 4. Формируем системный промпт
    std::string system = buildSystemPrompt(instr.normalized_name);

    // 5. Отправляем в Ollama
    auto resp = ollama_.generateChat(GEN_MODEL, system + "\n" + rag_context, user_query, 0.7f, 400);

    if (!resp.success) {
        return "Ошибка генерации ответа: " + resp.error;
    }

    return resp.content;
}

// Системный промпт (строго по ТЗ)
std::string Assistant::buildSystemPrompt(const std::string& instrument_name) const
{
    return
        "Ты — вежливый и точный ментор по работе с измерительными приборами.\n"
        "Сейчас подключён прибор: " + instrument_name + "\n"
        "Отвечай ТОЛЬКО на вопросы, связанные с этим прибором.\n"
        "Объясняй назначение кнопок, меню, параметров, давай рекомендации по настройке.\n"
        "НИКОГДА не пиши фразы «нажмите», «включите», «выполните команду» в повелительном наклонении.\n"
        "Если пользователь просит выполнить действие — отвечай:\n"
        "«Я только объясняю, как это работает. Выполните действие самостоятельно на приборе.»\n"
        "Если вопрос не относится к прибору — отвечай:\n"
        "«Я могу помочь только по работе с подключённым прибором.»\n"
        "Контекст из наиболее релевантных разделов руководства (найдены по смыслу запроса):\n";
}

// Сборка контекста из топ-N секций по cosine similarity
std::string Assistant::buildRagContext(
    const InstrumentData& instr,
    const std::vector<float>& query_emb,
    int top_k) const
{
    if (instr.sections.empty()) {
        return "Руководство по прибору не содержит разделов.\n";
    }

    // Собираем пары (секция, схожесть)
    std::vector<std::pair<const Section*, float>> scored;
    scored.reserve(instr.sections.size());

    for (const auto& sec : instr.sections) {
        if (sec.embedding.empty()) continue; // если эмбеддинг не был сгенерирован
        float sim = cosineSimilarity(query_emb, sec.embedding);
        scored.emplace_back(&sec, sim);
    }

    // Сортируем по убыванию схожести
    std::sort(scored.begin(), scored.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    // Берём топ-k
    std::string context;
    int count = 0;
    for (const auto& [sec, sim] : scored) {
        if (count >= top_k) break;
        context += "## " + sec->title + " (схожесть: " + std::to_string(sim) + ")\n";
        context += sec->content + "\n\n";
        ++count;
    }

    if (context.empty()) {
        context = "Релевантных разделов не найдено. Используй общие знания о приборе.\n";
    }

    return context;
}

// Cosine similarity (очень важная функция)
float Assistant::cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b)
{
    if (a.size() != b.size() || a.empty()) return 0.0f;

    float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }

    float denom = std::sqrt(norm_a) * std::sqrt(norm_b);
    if (denom < 1e-10f) return 0.0f;
    return dot / denom;
}