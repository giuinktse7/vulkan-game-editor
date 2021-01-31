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
    LuaGroundBrush() {}
    ~LuaGroundBrush()
    {
        VME_LOG("~LuaGroundBrush");
    }

    static void luaRegister(lua_State *L);

    static int luaCreate(lua_State *L);
    static int luaSetName(lua_State *L);
    static int luaGetName(lua_State *L);
    static int luaSetItemWeights(lua_State *L);

    void setName(const char *name);

    static LuaGroundBrush *checkSelf(lua_State *L);

    int x = 0;

  private:
    std::string _name;

    std::vector<WeightedItemId> weightedIds;
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
    static WeightedItemId parseItemWeight(lua_State *L);

    static int luaCreateGroundBrush(lua_State *L);

    static int createIndexTable(std::string metaTableId);
};
