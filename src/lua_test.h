#pragma once

#include <optional>
#include <string>

#include <lua.hpp>

class GroundBrush;

struct LuaUtil
{
    static int popInt(lua_State *L, bool *ok);
};

class LuaScriptInterface
{
  public:
    LuaScriptInterface();

    static bool initialize();
    static LuaScriptInterface *get();

    [[nodiscard]] lua_State *luaState() const noexcept;

    void test();

  private:
    LuaScriptInterface(const LuaScriptInterface &) = delete;
    LuaScriptInterface &operator=(const LuaScriptInterface &) = delete;

    bool initState();

    lua_State *L = nullptr;

    void errorAbort(const std::string &error);
    void stackDump();
};

class LuaParser
{
  public:
    static std::optional<GroundBrush> parseGroundBrush(lua_State *L);
};