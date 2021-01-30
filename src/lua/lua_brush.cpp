#include "lua_brush.h"

#include "../debug.h"
#include "lua_state.h"

int LuaGroundBrush::luaRegister(lua_State *L)
{
    // LuaState::stackDump(L);

    // Create the global table
    {
        lua_newtable(L);

        int globalObject = lua_gettop(L);

        const auto globalMetaTableName = (std::string(LuaName) + "__Meta").c_str();

        luaL_newmetatable(L, globalMetaTableName);

        luaL_Reg methods[] = {
            {"__call", luaCreate},
            {nullptr, nullptr}};

        luaL_setfuncs(L, methods, 0);
        lua_setmetatable(L, globalObject);

        lua_setglobal(L, LuaName);
    }

    // Source: https://stackoverflow.com/questions/26970316/lua-userdata-array-access-and-methods

    // LuaState::stackDump(L);

    // luaL_newmetatable(L, LuaId);

    // luaL_Reg methods[] = {
    //     {"__index", metaIndex},
    //     {nullptr, nullptr}};

    // luaL_Reg functions[] = {
    //     {"test", luaTest},
    //     {nullptr, nullptr}};

    // luaL_setfuncs(L, methods, 0);
    // luaL_setfuncs(L, functions, 0);
    // lua_pop(L, 1);

    // luaL_newlib(L, functions);

    // LuaState::stackDump(L);

    return 1;
}

int LuaGroundBrush::luaCreate(lua_State *L)
{
    auto brush = new LuaGroundBrush();

    LuaState::pushUserData(L, brush);
    LuaState::setMetaTable(L, -1, LuaId);

    VME_LOG_D("LuaGroundBrush::luaCreate");

    return 1;
}

int LuaGroundBrush::metaIndex(lua_State *L)
{
    LuaGroundBrush *brush = *static_cast<LuaGroundBrush **>(luaL_checkudata(L, 1, LuaGroundBrush::LuaId));

    luaL_getmetatable(L, LuaId);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);

    if (lua_isnil(L, -1))
    {
        /* found no method, so get value from userdata. */
        std::string index = luaL_checkstring(L, 2);

        luaL_argcheck(L, index == "x", 2, "Invalid index.");

        lua_pushinteger(L, brush->x);
    };

    return 1;
};

int LuaGroundBrush::luaTest(lua_State *L)
{
    LuaState::stackDump(L);

    LuaGroundBrush *brush = LuaState::checkUserData<LuaGroundBrush>(L, -1, LuaGroundBrush::LuaId);
    if (brush)
    {
        VME_LOG_D("Have brush!");
    }
    else
    {
        VME_LOG_D("no brush!");
    }
}

/**
  * Requires a ground brush table at the top of the stack.
  */
std::optional<GroundBrush> LuaBrush::parseGroundBrush(LuaState &L)
{
    DEBUG_ASSERT(L.isTable(-1), "Top of stack must be a table.");
    std::optional<GroundBrush> result = std::nullopt;

    auto brushName = L.readTableString(-1, "name");
    if (!brushName)
    {
        SHOW_LUA_ERROR("A ground brush needs key `name`, e.x. 'brush = { name = \"my brush\", ... }'");
        return result;
    }

    if (!L.getField(-1, "items", LuaType::Table))
    {
        SHOW_LUA_ERROR("A ground brush needs key `items` containing a list of items, e.x. items = {4526, 4527, 4528}.");
        return result;
    }

    std::vector<WeightedItemId> weightedIds;

    VME_LOG("Before:");
    L.stackDump();

    L.pushNil();
    while (L.next(-2))
    {
        auto weightedId = parseItemWeight(L);
        if (!weightedId)
        {
            SHOW_LUA_ERROR("Values in ground brush `items` list must be server IDs, e.x. 4526.");
            return result;
        }

        weightedIds.emplace_back(*weightedId);

        // Pop the weighted id table
        L.pop();
        L.stackDump();
    }

    // Pop the table
    L.pop();

    result.emplace(std::move(*brushName), std::move(weightedIds));

    return result;
}

std::optional<WeightedItemId> LuaBrush::parseItemWeight(LuaState &L)
{
    DEBUG_ASSERT(L.isTable(-1), "Top of stack must be a table.");
    std::optional<WeightedItemId> result = std::nullopt;

    if (!L.getField(-1, "id", LuaType::Integer))
    {
        SHOW_LUA_ERROR("id must be an integer.");
        return result;
    }
    int id = L.popIntUnsafe();

    if (!L.getField(-1, "weight", LuaType::Integer))
    {
        SHOW_LUA_ERROR("weight must be an integer.");
        return result;
    }
    int weight = L.popIntUnsafe();

    result.emplace(static_cast<uint32_t>(id), static_cast<uint32_t>(weight));

    return result;
}