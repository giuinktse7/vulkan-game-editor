#pragma once

#if __has_include("luajit/lua.hpp")
#include <luajit/lua.hpp>
#else
#include <lua.hpp>
#endif

#include <optional>
#include <string>

#include "lua_state.h"

class GroundBrush;
class LuaState;

class LuaScriptInterface
{
  public:
    LuaScriptInterface();

    static bool initialize();
    static LuaScriptInterface *get();

    void registerTable(const std::string &tableName);
    void registerMethod(const std::string &globalName, const std::string &methodName, lua_CFunction function);

    [[nodiscard]] lua_State *luaState() const noexcept;

    void test();

    static LuaScriptInterface g_luaInterface;

  private:
    LuaScriptInterface(const LuaScriptInterface &) = delete;
    LuaScriptInterface &operator=(const LuaScriptInterface &) = delete;

    bool initState();

    LuaState L;

    void errorAbort(const std::string &error);
};
