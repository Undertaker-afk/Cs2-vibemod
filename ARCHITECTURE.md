# 🎮 CS2 VibeMod — Architecture & Design Document

> An internally-injected CS2 DLL that embeds a LuaJIT scripting engine, exposes the full game API to Lua, and connects to an AI assistant so users can "vibecode" — describe what they want in plain English and have the AI write and hot-reload Lua mods live in-game.

---

## 📐 High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         cs2.exe (Game Process)                  │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    vibemod.dll  (injected)               │   │
│  │                                                          │   │
│  │  ┌─────────────┐   ┌──────────────┐  ┌───────────────┐  │   │
│  │  │  Hook Layer │   │  Lua Runtime │  │  AI Bridge    │  │   │
│  │  │  (MinHook)  │──▶│  (LuaJIT)   │◀─│  (HTTP/WS)    │  │   │
│  │  └─────────────┘   └──────────────┘  └───────────────┘  │   │
│  │         │                 │                  │           │   │
│  │  ┌──────▼──────┐  ┌──────▼──────┐   ┌───────▼───────┐  │   │
│  │  │ Sig Scanner │  │  Game API   │   │  Config Store │  │   │
│  │  │ (patterns)  │  │  Bindings   │   │  (JSON file)  │  │   │
│  │  └─────────────┘  └─────────────┘   └───────────────┘  │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                 │
│  client.dll / engine2.dll / materialsystem2.dll / tier0.dll     │
└─────────────────────────────────────────────────────────────────┘
         ▲ manual map inject
         │
┌────────┴──────────┐        ┌──────────────────────────────────┐
│   Loader.exe      │        │   External UI  (vibemod-ui/)     │
│  (ManualMapper)   │        │   - Electron / plain HTML+JS     │
│  - Finds cs2.exe  │        │   - Chat window with AI          │
│  - Maps DLL       │        │   - Lua code editor (Monaco)     │
│  - Starts thread  │        │   - Script library browser       │
└───────────────────┘        │   - Provider/API key config      │
                             │   Talks to DLL via named pipe    │
                             └──────────────────────────────────┘
```

---

## 🧩 Component Breakdown

### 1. `Loader.exe` — Manual Map Injector
- Finds `cs2.exe` by process name
- Reads `vibemod.dll` from disk into memory
- Manually maps it: resolves PE imports, applies relocations, calls TLS callbacks, invokes `DllMain`
- Uses **BlackBone** or a custom lightweight mapper
- Run as Administrator
- No `LoadLibrary` call → no entry in `PEB.InMemoryOrderModuleList` → stealthier

### 2. `vibemod.dll` — Core DLL

#### 2a. Hook Layer (`hooks/`)
Uses **MinHook** (x64) to detour CS2 functions found via signature scanning from `cspatterns.dev/json`:

| Hook | Purpose |
|---|---|
| `GameOverlayRenderer64.dll!present` | Render ImGui (fallback internal UI) |
| `client.dll!createmove` | Per-tick Lua callback (`on_createmove`) |
| `client.dll!framestagenotify` | Per-frame Lua callback (`on_frame`) |
| `client.dll!onaddentity` | Entity spawn Lua event |
| `client.dll!onremoveentity` | Entity remove Lua event |
| `client.dll!levelinit` | Map load Lua event |
| `client.dll!levelshutdown` | Map unload Lua event |
| `engine2.dll!isingame` | Expose `cs2.is_in_game()` to Lua |

#### 2b. Signature Scanner (`scanner/`)
- Loads patterns from embedded `patterns.json` (sourced from cspatterns.dev)
- Wildcard byte-pattern scanner over loaded module address ranges
- Resolves all needed function pointers at DLL init time
- Hot-reloadable if pattern file is updated on disk

#### 2c. LuaJIT Runtime (`lua/`)
- Embeds **LuaJIT 2.1** statically linked
- One persistent `lua_State*` per script slot (up to 8 concurrent scripts)
- Scripts can `require()` each other
- Sandboxed: `io`, `os`, `debug` libs restricted; replaced with safe CS2 equivalents
- Hot-reload: file watcher detects `.lua` changes and re-executes the script
- Error isolation: `lua_pcall` wraps every callback; errors logged, don't crash

#### 2d. Game API Bindings (`api/`)
Full Lua namespace `cs2.*` exposed via **LuaBridge3**:

```lua
-- Players
local me = cs2.local_player()
local players = cs2.get_players()       -- returns table of PlayerEntity
local hp = player:health()
local pos = player:position()           -- returns Vec3
local name = player:name()
local team = player:team()              -- 2=T, 3=CT
player:set_view_angles(pitch, yaw)

-- World
cs2.draw_text(x, y, text, r, g, b, a)
cs2.draw_box(x, y, w, h, r, g, b, a)
cs2.draw_line(x1, y1, x2, y2, r, g, b, a)
cs2.world_to_screen(vec3)               -- returns x, y or nil

-- Engine
cs2.is_in_game()
cs2.get_map_name()
cs2.send_chat(message)
cs2.get_tick()
cs2.get_curtime()

-- Input
cs2.is_key_down(keycode)

-- Events / Hooks
cs2.on("frame",       function() end)
cs2.on("createmove",  function(cmd) end)
cs2.on("entity_add",  function(ent) end)
cs2.on("entity_remove", function(ent) end)
cs2.on("map_load",    function(name) end)
cs2.on("map_unload",  function() end)

-- Script control
cs2.log(message)
cs2.load_script(path)
cs2.unload_script(name)
cs2.reload_script(name)
```

#### 2e. AI Bridge (`ai/`)
- Runs a lightweight **HTTP server on localhost:27015** (named pipe alternative)
- External UI connects to this
- Receives `{ prompt, context, provider, api_key, model }` JSON
- Builds a system prompt describing the full Lua API
- Forwards to the chosen provider (OpenAI / Anthropic / Ollama / custom)
- Streams response tokens back to UI
- On completion: extracts Lua code blocks, injects them as a new script or patches existing
- Supports providers:
  - **OpenAI** (`gpt-4o`, `gpt-4-turbo`)
  - **Anthropic** (`claude-3-5-sonnet`)
  - **Ollama** (`llama3`, `codestral`, etc.) — local, no key
  - **Custom OpenAI-compatible** endpoint

#### 2f. IPC / Named Pipe Server (`ipc/`)
- Windows Named Pipe: `\\.\pipe\vibemod`
- Bidirectional JSON-framed messages
- Messages:
  - `LIST_SCRIPTS` → returns all loaded script names + status
  - `LOAD_SCRIPT { code, name }` → compile + run Lua
  - `UNLOAD_SCRIPT { name }`
  - `AI_CHAT { prompt, provider, key, model }` → streams back tokens
  - `GET_LOGS` → returns recent log lines
  - `PING` → health check

### 3. `vibemod-ui/` — External UI (HTML/JS/Electron)
- Standalone window that sits alongside CS2
- **Monaco Editor** for Lua code with syntax highlighting
- **Chat panel**: type a prompt → AI writes Lua → one-click inject
- **Script browser**: see all running scripts, toggle, reload, delete
- **Provider config**: dropdown for OpenAI/Anthropic/Ollama + API key field (stored in `config.json`, never sent to our servers)
- **Console/log panel**: live streaming of `cs2.log()` output
- Communicates with DLL via Named Pipe

---

## 📁 Project Structure

```
cs2-vibemod/
├── ARCHITECTURE.md              ← this file
│
├── loader/                      ← Loader.exe (C++)
│   ├── main.cpp
│   ├── manual_map.hpp
│   ├── manual_map.cpp
│   └── CMakeLists.txt
│
├── vibemod/                     ← vibemod.dll (C++)
│   ├── dllmain.cpp
│   ├── CMakeLists.txt
│   │
│   ├── hooks/
│   │   ├── hook_manager.hpp
│   │   ├── hook_manager.cpp
│   │   ├── present_hook.hpp/.cpp
│   │   ├── createmove_hook.hpp/.cpp
│   │   └── framestage_hook.hpp/.cpp
│   │
│   ├── scanner/
│   │   ├── sig_scanner.hpp
│   │   ├── sig_scanner.cpp
│   │   └── patterns.json        ← from cspatterns.dev/json
│   │
│   ├── lua/
│   │   ├── lua_runtime.hpp
│   │   ├── lua_runtime.cpp
│   │   ├── lua_sandbox.hpp/.cpp
│   │   └── script_manager.hpp/.cpp
│   │
│   ├── api/
│   │   ├── cs2_api.hpp
│   │   ├── cs2_api.cpp          ← LuaBridge3 bindings
│   │   ├── entity_api.hpp/.cpp
│   │   ├── draw_api.hpp/.cpp
│   │   └── engine_api.hpp/.cpp
│   │
│   ├── ai/
│   │   ├── ai_bridge.hpp
│   │   ├── ai_bridge.cpp
│   │   ├── providers/
│   │   │   ├── openai_provider.hpp/.cpp
│   │   │   ├── anthropic_provider.hpp/.cpp
│   │   │   └── ollama_provider.hpp/.cpp
│   │   └── system_prompt.hpp    ← baked-in API description for the AI
│   │
│   ├── ipc/
│   │   ├── pipe_server.hpp
│   │   └── pipe_server.cpp
│   │
│   └── vendor/
│       ├── luajit/              ← LuaJIT 2.1 static lib
│       ├── luabridge3/          ← header-only
│       ├── minhook/             ← MinHook x64
│       ├── nlohmann/            ← json.hpp
│       └── httplib/             ← cpp-httplib (header-only)
│
├── vibemod-ui/                  ← External UI
│   ├── index.html
│   ├── style.css
│   ├── app.js
│   ├── pipe-client.js
│   ├── monaco-editor/
│   └── package.json            ← (optional Electron wrapper)
│
└── examples/                   ← Example Lua scripts
    ├── esp.lua
    ├── bhop.lua
    ├── triggerbot.lua
    └── radar.lua
```

---

## 🔄 Vibecoding Flow

```
User types: "make a green ESP box around enemy players"
                │
                ▼
         vibemod-ui chat panel
                │
                ▼
    POST /ai { prompt: "...", provider: "openai", model: "gpt-4o" }
                │
                ▼
         AI Bridge (in DLL)
          builds system prompt with full cs2.* API docs
                │
                ▼
         OpenAI API (streamed)
          returns Lua code block
                │
                ▼
         UI shows code in Monaco editor
         User clicks ▶ "Inject"
                │
                ▼
         Pipe: LOAD_SCRIPT { name: "ai_esp", code: "..." }
                │
                ▼
         LuaJIT compiles & runs the script
         Script registers cs2.on("frame", ...) callback
                │
                ▼
         Every frame: hook fires → Lua callback runs → draws boxes
```

---

## 🛡️ Safety Notes

- This project is for **educational / research / private server** use only
- Playing on VAC-secured servers with this is against Steam ToS and will result in a ban
- The DLL does not exfiltrate any data; AI API calls are made directly from the DLL to the provider using the user's own key
- All network calls are localhost (pipe) or outbound to the AI API only

---

## 🔧 Build Requirements

| Tool | Version |
|---|---|
| Visual Studio | 2022 (MSVC v143) |
| Windows SDK | 10.0.22621+ |
| CMake | 3.25+ |
| LuaJIT | 2.1.0-beta3 |
| MinHook | 1.3.3 |
| LuaBridge3 | 3.0+ |
| nlohmann/json | 3.11+ |
| cpp-httplib | 0.14+ |

---

## 🚀 Roadmap

- [ ] Phase 1: DLL skeleton + manual mapper + named pipe IPC
- [ ] Phase 2: Signature scanner + hook layer (createmove, framestage, present)
- [ ] Phase 3: LuaJIT embed + sandbox + script manager + hot-reload
- [ ] Phase 4: Game API bindings (players, drawing, engine)
- [ ] Phase 5: AI bridge + provider implementations + system prompt
- [ ] Phase 6: External UI (HTML) + pipe client
- [ ] Phase 7: Example scripts + documentation
- [ ] Phase 8: Electron wrapper + installer
