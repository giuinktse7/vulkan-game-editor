
#include "luascript_interface.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <initializer_list>
#include <iostream>
#include <string>

#include "../brushes/brush.h"
#include "../brushes/ground_brush.h"
#include "../debug.h"
#include "../time_point.h"

#include "lua_brush.h"

LuaScriptInterface LuaScriptInterface::g_luaInterface;

LuaScriptInterface::LuaScriptInterface() {}

bool LuaScriptInterface::initialize()
{
    return g_luaInterface.initState();
}

bool LuaScriptInterface::initState()
{
    return L.initialize();
}

void LuaScriptInterface::errorAbort(const std::string &error)
{
    VME_LOG_ERROR(error);
    L.close();
    exit(EXIT_FAILURE);
}

// bool getDoodadBrush(lua_State *L) {

// }

LuaScriptInterface *LuaScriptInterface::get()
{
    return &g_luaInterface;
}

void LuaScriptInterface::test()
{
    // sol::state lua;
    // lua.open_libraries(sol::lib::base, sol::lib::package);
    // lua.script("print('test from lua')");

    // TODO Continue here? https://stackoverflow.com/questions/52232478/how-to-index-a-converted-userdata-value
    // TODO And maybe here.. https://eliasdaler.github.io/game-object-references/

    // L.registerClass(LuaGroundBrush::LuaId, "", LuaGroundBrush::luaCreate);
    // L.testMetaTables();
    // L.registerMethod(LuaGroundBrush::LuaId, "test", LuaGroundBrush::luaTest);
    LuaGroundBrush::luaRegister(L.L);

    if (!L.doFile("test.lua"))
    {
        VME_LOG("[Error - Lua load] " << L.toString(-1));
        L.close();
        return;
    }

    if (L.getGlobal("brush") == LuaType::UserData)
    {
        VME_LOG("Yep!");
    }

    L.close();
    return;

    TimePoint start;
    if (L.getGlobal("groundBrush") == LuaType::Table)
    {
        LuaBrush::parseGroundBrush(L);
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

    L.close();
}

void LuaScriptInterface::registerTable(const std::string &tableName)
{
    L.registerTable(tableName);
}

void LuaScriptInterface::registerMethod(const std::string &globalName, const std::string &methodName, lua_CFunction function)
{
    L.registerMethod(globalName, methodName, function);
}

lua_State *LuaScriptInterface::luaState() const noexcept
{
    return L.L;
}
