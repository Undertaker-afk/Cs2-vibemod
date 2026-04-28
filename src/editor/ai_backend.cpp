#include "ai_backend.hpp"
#include <windows.h>
#include <wininet.h>
#include <iostream>
#include <sstream>

#pragma comment(lib, "wininet.lib")

AIBackend::AIBackend(const Config& config) : cfg(config) {}

std::string HttpPost(const std::string& host, const std::string& path, const std::string& payload, const std::string& apiKey) {
    HINTERNET hSession = InternetOpenA("VibeCoder", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hSession) return "";

    HINTERNET hConnect = InternetConnectA(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1);
    if (!hConnect) { InternetCloseHandle(hSession); return ""; }

    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", path.c_str(), NULL, NULL, NULL, INTERNET_FLAG_SECURE, 1);
    if (!hRequest) { InternetCloseHandle(hConnect); InternetCloseHandle(hSession); return ""; }

    std::string headers = "Content-Type: application/json\r\nAuthorization: Bearer " + apiKey;

    if (HttpSendRequestA(hRequest, headers.c_str(), headers.length(), (LPVOID)payload.c_str(), payload.length())) {
        char buffer[4096];
        DWORD bytesRead;
        std::string response;
        while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            response.append(buffer, bytesRead);
        }
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hSession);
        return response;
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hSession);
    return "";
}

std::string AIBackend::Ask(const std::string& prompt, const std::string& systemContext) {
    // Simple OpenAI API call
    json payload = {
        {"model", cfg.model},
        {"messages", json::array({
            {{"role", "system"}, {"content", systemContext}},
            {{"role", "user"}, {"content", prompt}}
        })}
    };

    // Extracting host and path from cfg.endpoint (very simplified)
    // Assuming endpoint is like "api.openai.com" and we add "/v1/chat/completions"
    std::string response = HttpPost("api.openai.com", "/v1/chat/completions", payload.dump(), cfg.apiKey);

    try {
        auto j = json::parse(response);
        return j["choices"][0]["message"]["content"];
    } catch (...) {
        return "-- Error calling AI: " + response;
    }
}
