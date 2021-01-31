
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
    // TODO Continue here? https://stackoverflow.com/questions/52232478/how-to-index-a-converted-userdata-value
    // TODO And maybe here.. https://eliasdaler.github.io/game-object-references/

    LuaGroundBrush::luaRegister(L.L);

    if (!L.doFile("test.lua"))
    {
        VME_LOG("[Error - Lua load] " << L.toString(-1));
        L.close();
        return;
    }

    VME_LOG("Finished running test.lua.");

    L.close();
}

void LuaScriptInterface::registerTable(const std::string &tableName)
{
    L.registerTable(tableName);
}

// void LuaScriptInterface::registerMethod(const std::string &globalName, const std::string &methodName, lua_CFunction function)
// {
//     L.registerMethod(globalName, methodName, function);
// }

lua_State *LuaScriptInterface::luaState() const noexcept
{
    return L.L;
}
