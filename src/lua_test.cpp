
#include "lua_test.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <initializer_list>
#include <iostream>
#include <string>

#include "./debug.h"
#include "brushes/ground_brush.h"
#include "time_point.h"

#define EXIT_LUA_ERROR(errorString) errorAbort(FILE_AND_LINE_STR + errorString)
#define SHOW_LUA_ERROR(errorString) VME_LOG_ERROR(FILE_AND_LINE_STR + errorString)

LuaScriptInterface g_luaInterface;

// static
bool LuaScriptInterface::initialize()
{
    return g_luaInterface.initState();
}

LuaScriptInterface::LuaScriptInterface() {}

bool LuaScriptInterface::initState()
{
    L = luaL_newstate();
    if (!L)
    {
        VME_LOG_ERROR(FILE_AND_LINE_STR + "Not enough memory to create Lua state.");
        return false;
    }

    luaL_openlibs(L);

    VME_LOG("LuaScriptInterface initialized.");

    return true;
}

void LuaScriptInterface::errorAbort(const std::string &error)
{
    VME_LOG_ERROR(error);
    lua_close(L);
    exit(EXIT_FAILURE);
}

// bool getDoodadBrush(lua_State *L) {

// }

LuaScriptInterface *LuaScriptInterface::get()
{
    return &g_luaInterface;
}

std::optional<GroundBrush> LuaParser::parseGroundBrush(lua_State *L)
{
    // Requires ground brush table at top of stack.
    std::optional<GroundBrush> result = std::nullopt;

    lua_getfield(L, -1, "name");
    if (!lua_isstring(L, -1))
    {
        SHOW_LUA_ERROR("A ground brush needs key `name`, e.x. 'brush = { name = \"my brush\", ... }'");
        return result;
    }

    std::string name = lua_tostring(L, -1);
    lua_pop(L, 1);

    std::vector<uint32_t> ids;
    lua_getfield(L, -1, "items");
    if (!lua_istable(L, -1))
    {
        SHOW_LUA_ERROR("A ground brush needs key `items` containing a list of items, e.x. items = {4526, 4527, 4528}.");
        return result;
    }

    for (lua_pushnil(L); lua_next(L, -2) != 0;)
    {
        bool ok;
        uint32_t serverId = static_cast<uint32_t>(LuaUtil::popInt(L, &ok));
        if (!ok)
        {
            SHOW_LUA_ERROR("Values in ground brush `items` list must be server IDs, e.x. 4526.");
            return result;
        }
        ids.emplace_back(serverId);
    }

    return result;
}

int LuaUtil::popInt(lua_State *L, bool *ok)
{
    if (!lua_isnumber(L, -1))
    {
        *ok = false;
        return -1;
    }

    lua_Number luaNumber = lua_tonumber(L, -1);
    if (trunc(luaNumber) != luaNumber)
    {
        *ok = false;
        return -1;
    }

    *ok = true;
    lua_pop(L, 1);
    return static_cast<int>(luaNumber);
}

void LuaScriptInterface::test()
{
    // sol::state lua;
    // lua.open_libraries(sol::lib::base, sol::lib::package);
    // lua.script("print('test from lua')");

    if (luaL_dofile(L, "test.lua"))
    {
        VME_LOG("[Error - Lua load] " << lua_tostring(L, -1));
        lua_close(L);
        return;
    }

    TimePoint start;
    lua_getglobal(L, "groundBrush");

    if (lua_istable(L, -1))
    {
        LuaParser::parseGroundBrush(L);
    }
    else if (!lua_isnumber(L, -1))
    {
        // Number
        VME_LOG("number");
    }
    else
    {
        EXIT_LUA_ERROR("background is not a table nor a number.");
    }
    VME_LOG_D(std::to_string(start.elapsedMicros()) + " us");

    // stackDump(L);

    // lua_pushnil(L); // put a nil key on stack
    // while (lua_next(L, -2) != 0)
    // {                                    // key(-1) is replaced by the next key(-1) in table(-2)
    //     auto name = lua_tostring(L, -2); // Get key(-2) name
    //     VME_LOG(name);
    //     lua_pop(L, 1); // remove value(-1), now key on top at(-1)
    // }
    // lua_pop(L, 1); // remove global table(-1)

    // lua_getglobal(L, "width");
    // lua_getglobal(L, "height");
    // if (!lua_isnumber(L, -2))
    // {
    //     VME_LOG("rip 1");
    //     return;
    // }
    // if (!lua_isnumber(L, -1))
    // {
    //     VME_LOG("rip 2");
    //     return;
    // }

    lua_close(L);
}

lua_State *LuaScriptInterface::luaState() const noexcept
{
    return L;
}

void LuaScriptInterface::stackDump()
{
    int i;
    int top = lua_gettop(L);
    for (i = 1; i <= top; i++)
    { /* repeat for each level */
        int t = lua_type(L, i);
        switch (t)
        {

            case LUA_TSTRING: /* strings */
                printf("`%s'", lua_tostring(L, i));
                break;

            case LUA_TBOOLEAN: /* booleans */
                printf(lua_toboolean(L, i) ? "true" : "false");
                break;

            case LUA_TNUMBER: /* numbers */
                printf("%g", lua_tonumber(L, i));
                break;

            default: /* other values */
                printf("%s", lua_typename(L, t));
                break;
        }
        printf("  "); /* put a separator */
    }
    printf("\n"); /* end the listing */
}

#undef EXIT_LUA_ERROR