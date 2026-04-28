#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class AIBackend {
public:
    struct Config {
        std::string provider; // openai, anthropic, ollama
        std::string apiKey;
        std::string endpoint;
        std::string model;
    };

    AIBackend(const Config& config);
    std::string Ask(const std::string& prompt, const std::string& systemContext);

private:
    Config cfg;
    std::string CallOpenAI(const std::string& prompt, const std::string& systemContext);
    // ... other providers
};
