// Мозг проекта - отвечает за всё
// Главная логика AI-ассистента (берёт данные / составляет промпт / отправляет Ollama / получает ответ)

#include "Assistant.h"
#include <iostream>
#include <cmath>
#include <limits>

Assistant::Assistant(
    const std::string& mongo_uri,
    const std::string& ollama_url,
    const std::string& default_model)
    : mongo_(mongo_uri, ollama_url),
    ollama_(ollama_url),
    current_gen_model_(default_model)
{
    MongoDBHelper::init();
    if (!mongo_.isConnected()) {
        std::cerr << "[Assistant] MongoDB не подключён!\n";
    }
    std::cout << "[Assistant] Запущен с моделью: " << current_gen_model_ << "\n";
}

void Assistant::setGenerationModel(const std::string& model_name)
{
    current_gen_model_ = model_name;
    std::cout << "[Assistant] Модель генерации переключена на: " << current_gen_model_ << "\n";
}

std::string Assistant::getCurrentModel() const
{
    return current_gen_model_;
}

// Главный метод
std::string Assistant::getResponse(const std::string& user_query, const std::string& device_id)
{
    auto opt_instr = mongo_.findInstrument(device_id);
    if (!opt_instr) {
        return "Устройство не распознано. Пожалуйста, загрузите руководство пользователя в формате PDF или DOCX.";
    }

    const auto& instr = *opt_instr;

    // Генерация эмбеддинга запроса
    auto emb_res = ollama_.generateEmbedding(EMBED_MODEL, user_query);
    if (!emb_res.success) {
        return "Ошибка генерации эмбеддинга: " + emb_res.error;
    }

    const auto& query_emb = emb_res.vector;

    // Сборка контекста
    std::string rag_context = buildRagContext(instr, query_emb, 3);

    // Системный промпт
    std::string system = buildSystemPrompt(instr.normalized_name);

    // Генерация ответа с выбранной моделью
    auto resp = ollama_.generateChat(
        current_gen_model_,           // ← используем выбранную модель
        system + "\n" + rag_context,
        user_query,
        0.7f,
        800
    );

    if (!resp.success) {
        return "Ошибка генерации ответа: " + resp.error;
    }

    return resp.content;
}

// Системный промпт
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
        "Контекст из наиболее релевантных разделов руководства (найдены по смыслу запроса):\n";
}

// RAG-контекст
std::string Assistant::buildRagContext(
    const InstrumentData& instr,
    const std::vector<float>& query_embedding,
    int top_k) const
{
    std::string context;

    // 1. Семантический поиск по секциям руководства (RAG)
    if (!instr.sections.empty()) {
        std::vector<std::pair<const Section*, float>> scored;
        scored.reserve(instr.sections.size());

        for (const auto& sec : instr.sections) {
            if (sec.embedding.empty()) continue;
            float sim = cosineSimilarity(query_embedding, sec.embedding);
            scored.emplace_back(&sec, sim);
        }

        // Сортируем по убыванию схожести
        std::sort(scored.begin(), scored.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        context += "=== РЕЛЕВАНТНЫЕ РАЗДЕЛЫ РУКОВОДСТВА ===\n\n";

        int count = 0;
        for (const auto& [sec, sim] : scored) {
            if (count >= top_k) break;
            context += "## " + sec->title + "\n";
            context += sec->content + "\n\n";
            ++count;
        }
    }

    // 2. Добавляем ВСЕ доступные команды (это очень важно!)
    if (!instr.commands.empty()) {
        context += "=== ДОСТУПНЫЕ КОМАНДЫ ПРИБОРА ===\n\n";

        for (const auto& [key, cmd] : instr.commands) {
            context += "• " + key + " — " + cmd.description + "\n";
            if (!cmd.scpi.empty()) {
                context += "  SCPI: " + cmd.scpi + "\n";
            }
            context += "\n";
        }
    }

    // Если ничего не нашлось
    if (context.empty()) {
        context = "Информация по данному прибору ограничена.\n";
    }

    return context;
}
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