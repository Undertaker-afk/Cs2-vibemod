#include <windows.h>
#include <iostream>
#include <string>
#include "zdraw.hpp"
#include "zui/zui.hpp"
#include "pipe_client.hpp"
#include "ai_backend.hpp"

PipeClient g_Client("VibeCoderPipe");
AIBackend* g_AI = nullptr;

char g_CodeBuffer[1024 * 64] = "-- Welcome to VibeCoder\n-- Type your prompt below to generate code";
char g_PromptBuffer[1024] = "Create a bhop script";

void RenderUI() {
    zui::begin_window("VibeCoder Editor", {100, 100, 800, 600});

    zui::columns(2);

    // Left side: Code Editor
    zui::text("Lua Script:");
    zui::input_text_multiline("##editor", g_CodeBuffer, sizeof(g_CodeBuffer), {400, 450});

    if (zui::button("Run Script")) {
        g_Client.Send("EXEC:" + std::string(g_CodeBuffer));
    }

    zui::next_column();

    // Right side: AI & Settings
    zui::text("Vibe with AI:");
    zui::input_text("##prompt", g_PromptBuffer, sizeof(g_PromptBuffer));

    if (zui::button("Ask AI")) {
        if (g_AI) {
            std::string code = g_AI->Ask(g_PromptBuffer, "You are a CS2 Lua script generator.");
            strncpy(g_CodeBuffer, code.c_str(), sizeof(g_CodeBuffer));
        }
    }

    zui::separator();
    zui::text("Settings");
    // Provider selection, API keys, etc.

    zui::end_window();
}

int main() {
    // Initialize zdraw and zui
    // Loop and call RenderUI
    std::cout << "[VibeCoder] External Editor Started." << std::endl;

    AIBackend::Config cfg = {"openai", "YOUR_KEY", "https://api.openai.com/v1", "gpt-4"};
    g_AI = new AIBackend(cfg);

    // Simulation loop
    while (true) {
        // In a real app, this would be the Win32 message loop + zdraw rendering
        Sleep(16);
    }

    return 0;
}
