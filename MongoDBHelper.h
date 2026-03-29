#pragma once

#include "types.h"
#include <string>
#include "OllamaClient.h"
#include <optional>

// Корневые заголовки — vcpkg сам подставит v_noabi
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/document/view.hpp>

class MongoDBHelper {
public:
    explicit MongoDBHelper(
        const std::string& uri = "mongodb://localhost:27017",
        const std::string& ollama_url = "http://127.0.0.1:11434");

    static void init();
    std::optional<InstrumentData> findInstrument(const std::string& device_id);
    bool insertInstrument(const InstrumentData& data);
    bool isConnected() const;

private:
    mongocxx::client client_;
    static mongocxx::instance instance_;
    OllamaClient ollama_;

    InstrumentData bson_to_instrument(const bsoncxx::document::view& doc) const;
};