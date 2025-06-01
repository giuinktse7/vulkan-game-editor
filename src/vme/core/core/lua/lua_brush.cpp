#include "lua_brush.h"

#include "../debug.h"
#include "lua_state.h"

void LuaGroundBrush::luaRegister(lua_State *L)
{
    LuaState::registerClass(L, LuaName, luaCreate);

    LuaState::registerField(L, LuaName, "name", luaGetName);
    LuaState::registerFunction(L, LuaName, "setName", luaSetName);

    LuaState::registerFunction(L, LuaName, "setItemWeights", luaSetItemWeights);

    auto k = [](lua_State *L) { lua_pushstring(L, "'result of lambdaFn'"); return 1; };
    LuaState::registerField(L, LuaName, "lambdaFn", k);
}

int LuaGroundBrush::luaCreate(lua_State *L)
{
    auto *brush = new LuaGroundBrush();

    LuaState::pushUserData(L, brush);
    LuaState::setMetaTable(L, -1, LuaName);

    return 1;
}

LuaGroundBrush *LuaGroundBrush::checkSelf(lua_State *L)
{
    return LuaState::checkUserData<LuaGroundBrush>(L, 1, LuaGroundBrush::LuaName);
}

int LuaGroundBrush::luaGetName(lua_State *L)
{
    auto *self = checkSelf(L);

    lua_pushstring(L, self->_name.c_str());

    return 1;
}

int LuaGroundBrush::luaSetName(lua_State *L)
{
    auto *self = checkSelf(L);
    const auto *name = luaL_checkstring(L, 2);

    self->setName(name);

    return 0;
}

int LuaGroundBrush::luaSetItemWeights(lua_State *L)
{
    auto *self = checkSelf(L);
    luaL_argcheck(L, lua_istable(L, 2), 1, "Expected a table.");

    lua_pushnil(L);
    while (lua_next(L, -2))
    {
        WeightedItemId weightedId = LuaBrush::parseItemWeight(L);

        self->weightedIds.emplace_back(weightedId);

        // Pop the weighted id table
        lua_pop(L, 1);
    }

    // Pop the entire table
    lua_pop(L, 1);

    return 0;
}

void LuaGroundBrush::setName(const char *name)
{
    _name = std::string(name);
}

WeightedItemId LuaBrush::parseItemWeight(lua_State *L)
{
    luaL_argcheck(L, lua_istable(L, -1), 1, "Argument must be a table.");

    int id = LuaState::checkIntField(L, "id");
    int weight = LuaState::checkIntField(L, "weight");

    return WeightedItemId(id, weight);
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