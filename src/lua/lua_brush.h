#pragma once

#include <optional>
#include <vector>

#include "../brushes/brush.h"
#include "../brushes/ground_brush.h"

#include "../debug.h"

class LuaState;
struct lua_State;

class LuaGroundBrush
{
  public:
    static constexpr auto LuaName = "GroundBrush";
    static constexpr auto LuaId = "GroundBrushMeta";
    static constexpr auto LuaConstructorId = "GroundBrushConstructor";
    LuaGroundBrush() {}
    ~LuaGroundBrush()
    {
        VME_LOG("~LuaGroundBrush");
    }

    static int luaRegister(lua_State *L);

    static int luaCreate(lua_State *L);

    static int luaTest(lua_State *L);

    static int metaIndex(lua_State *L);

    int x = 0;
};

class LuaBrush
{
  public:
    /**
      * Requires a ground brush table at the top of the stack.
      */
    static std::optional<GroundBrush> parseGroundBrush(LuaState &L);

    /**
     * Parses format { id=<uint>, weight=<uint> }
     */
    static std::optional<WeightedItemId> parseItemWeight(LuaState &L);

    static int luaCreateGroundBrush(lua_State *L);

    static int createIndexTable(std::string metaTableId);
};
