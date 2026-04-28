#pragma once
#include <lua.hpp>
#include <string>
#include <functional>
#include <map>

class LuaEngine {
public:
    LuaEngine();
    ~LuaEngine();

    void ExecuteString(const std::string& code);
    void RegisterFunction(const char* name, lua_CFunction func);

private:
    lua_State* L;
    void SetupAPI();
};
