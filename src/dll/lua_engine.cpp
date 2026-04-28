#include "lua_engine.hpp"
#include <iostream>

LuaEngine::LuaEngine() {
    L = luaL_newstate();
    luaL_openlibs(L);
    SetupAPI();
}

LuaEngine::~LuaEngine() {
    lua_close(L);
}

void LuaEngine::ExecuteString(const std::string& code) {
    if (luaL_dostring(L, code.c_str()) != LUA_OK) {
        std::cerr << "Lua Error: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
    }
}

void LuaEngine::RegisterFunction(const char* name, lua_CFunction func) {
    lua_register(L, name, func);
}

// Example API functions
int l_MemoryReadInt(lua_State* L) {
    uintptr_t addr = (uintptr_t)lua_tonumber(L, 1);
    int value = *(int*)addr;
    lua_pushinteger(L, value);
    return 1;
}

void LuaEngine::SetupAPI() {
    // Create 'Game' table
    lua_newtable(L);

    lua_pushcfunction(L, l_MemoryReadInt);
    lua_setfield(L, -2, "ReadInt");

    // Add more game functions here...

    lua_setglobal(L, "Game");
}
