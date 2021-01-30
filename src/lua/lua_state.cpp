#include "lua_state.h"

#include "../debug.h"
#include "../util.h"
#include "lua_brush.h"

LuaState::LuaState() {}

LuaState::LuaState(lua_State *L)
    : L(L) {}

bool LuaState::registerTable(const std::string &tableName)
{
    if (getGlobal(tableName) != LuaType::Nil)
    {
        SHOW_LUA_ERROR("A global table with name " << tableName << " already exists.");
        return false;
    }

    lua_newtable(L);
    lua_setglobal(L, tableName.c_str());
    return true;
}

LuaType LuaState::getField(int stackOffset, const char *name)
{
    return static_cast<LuaType>(lua_getfield(L, stackOffset, name));
}

LuaType LuaState::getField(lua_State *L, int stackOffset, const char *name)
{
    return static_cast<LuaType>(lua_getfield(L, stackOffset, name));
}

bool LuaState::getField(int stackOffset, const char *name, LuaType requiredType)
{
    LuaType luaType = getField(stackOffset, name);
    if (requiredType == LuaType::Integer)
    {
        return lua_isinteger(L, -1);
    }
    else
    {
        return luaType == requiredType;
    }
}

std::optional<std::string> LuaState::readTableString(int tableStackOffset, const char *name)
{
    if (!getField(tableStackOffset, name, LuaType::String))
    {
        return std::nullopt;
    }

    std::optional<std::string> value = popStringUnsafe();
    return value;
}

LuaType LuaState::getGlobal(const char *name)
{
    return static_cast<LuaType>(lua_getglobal(L, name));
}

LuaType LuaState::getGlobal(const std::string &name)
{
    return static_cast<LuaType>(lua_getglobal(L, name.c_str()));
}

int LuaState::topIndex() const
{
    return lua_gettop(L);
}

int LuaState::topIndex(lua_State *L)
{
    return lua_gettop(L);
}

LuaType LuaState::type(int stackOffset) const
{
    return static_cast<LuaType>(lua_type(L, stackOffset));
}

LuaType LuaState::type(lua_State *L, int stackOffset)
{
    return static_cast<LuaType>(lua_type(L, stackOffset));
}

std::string LuaState::toString(int stackOffset)
{
    return lua_tostring(L, stackOffset);
}

std::string LuaState::toString(lua_State *L, int stackOffset)
{
    return lua_tostring(L, stackOffset);
}

bool LuaState::toBool(int stackOffset)
{
    return lua_toboolean(L, stackOffset);
}

bool LuaState::toBool(lua_State *L, int stackOffset)
{
    return lua_toboolean(L, stackOffset);
}

lua_Number LuaState::toNumber(int stackOffset)
{
    return lua_tonumber(L, stackOffset);
}

lua_Number LuaState::toNumber(lua_State *L, int stackOffset)
{
    return lua_tonumber(L, stackOffset);
}

std::string LuaState::typeName(LuaType type) const
{
    return lua_typename(L, static_cast<std::underlying_type_t<LuaType>>(type));
}

std::string LuaState::typeName(lua_State *L, LuaType type)
{
    return lua_typename(L, static_cast<std::underlying_type_t<LuaType>>(type));
}

void LuaState::pushNil()
{
    lua_pushnil(L);
}

void LuaState::pushNumber(lua_Number number)
{
    lua_pushnumber(L, number);
}

bool LuaState::doFile(std::string file)
{
    return luaL_dofile(L, file.c_str()) == 0;
}

bool LuaState::next(int stackOffset)
{
    return lua_next(L, stackOffset) != 0;
}

int LuaState::popIntUnsafe()
{
    lua_Number luaNumber = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return static_cast<int>(luaNumber);
}

int LuaState::popInt(bool *ok)
{
    if (!isNumber(-1))
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

std::optional<std::string> LuaState::popString()
{
    std::optional<std::string> result = std::nullopt;

    return isString(-1) ? std::optional<std::string>(popStringUnsafe()) : std::nullopt;
}

std::string LuaState::popStringUnsafe()
{
    std::string s = lua_tostring(L, -1);
    pop();

    return s;
}

bool LuaState::initialize()
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

void LuaState::pop(int amount)
{
    lua_pop(L, amount);
}

void LuaState::close()
{
    lua_close(L);
    L = nullptr;
}

bool LuaState::isTable(int stackOffset) const
{
    return lua_istable(L, stackOffset);
}

bool LuaState::isNumber(int stackOffset) const
{
    return lua_isnumber(L, stackOffset);
}

bool LuaState::isString(int stackOffset) const
{
    return lua_isstring(L, stackOffset);
}

void LuaState::setFieldUnsafe(int stackOffset, const char *name)
{
    lua_setfield(L, stackOffset, name);
}

int setIndexTest(lua_State *L)
{
    LuaGroundBrush *brush = *static_cast<LuaGroundBrush **>(luaL_checkudata(L, 1, LuaGroundBrush::LuaId));

    luaL_argcheck(L, brush != nullptr, 1, "invalid pointer");
    std::string index = luaL_checkstring(L, 2);
    luaL_argcheck(L, index == "x", 2, "Invalid index.");
    luaL_argcheck(L, lua_isnumber(L, 3), 3, "Not a number.");
    float x = lua_tonumber(L, 3);

    brush->x = x;

    return 0;
}

int LuaState::indexTest(lua_State *L)
{
    // TODO Continue here: https://stackoverflow.com/questions/26970316/lua-userdata-array-access-and-methods
    LuaGroundBrush *brush = *static_cast<LuaGroundBrush **>(luaL_checkudata(L, 1, LuaGroundBrush::LuaId));

    luaL_argcheck(L, brush != nullptr, 1, "invalid pointer");

    std::string index = luaL_checkstring(L, 2);
    if (index == "x")
    {
        lua_pushnumber(L, brush->x);

        VME_LOG_D("indexTest: " << brush->x);

        return 1;
    }
    else
    {
        stackDump(L);

        lua_getglobal(L, LuaGroundBrush::LuaId);
        auto k = getField(L, -1, index.c_str());

        // lua_replace(L, -2);
        // lua_replace(L, -2);

        lua_remove(L, -2);
        lua_remove(L, -2);
        stackDump(L);
    }
}

void LuaState::testMetaTables()
{
    luaL_getmetatable(L, LuaGroundBrush::LuaId);

    luaL_Reg meta[] = {
        {LuaMetaTable::NewIndex, setIndexTest},
        {LuaMetaTable::Index, indexTest},
        {nullptr, nullptr}};

    luaL_setfuncs(L, meta, 0);
    lua_pop(L, 1);
}

void LuaState::registerClass2(const std::string &className, lua_CFunction constructor)
{
}

void LuaState::registerClass(const std::string &className, const std::string &baseClass, lua_CFunction constructor)
{
    lua_newtable(L);
    int methodsOffset = topIndex();

    pushCopy(-1);
    lua_setglobal(L, className.c_str());

    lua_newtable(L);
    int methodsMetaTableOffset = topIndex();

    if (constructor)
    {
        lua_pushcfunction(L, constructor);
        lua_setfield(L, methodsMetaTableOffset, "__call");
    }

    uint32_t parents = 0;
    if (!baseClass.empty())
    {
        getGlobal(baseClass.c_str());
        lua_rawgeti(L, -1, to_underlying(ClassMetaTableIndex::Parent));
        parents = popIntUnsafe() + 1;
        setFieldUnsafe(methodsMetaTableOffset, LuaMetaTable::Index);
    }

    setMetaTable(methodsOffset);

    luaL_newmetatable(L, className.c_str());
    int metaTableOffset = topIndex();

    pushCopy(methodsOffset);
    setFieldUnsafe(metaTableOffset, LuaMetaTable::MetaTable);

    pushCopy(methodsOffset);
    setFieldUnsafe(metaTableOffset, LuaMetaTable::Index);

    size_t hash = std::hash<std::string>()(className);
    pushNumber(hash);
    lua_rawseti(L, metaTableOffset, to_underlying(ClassMetaTableIndex::Hash));

    pushNumber(parents);
    lua_rawseti(L, metaTableOffset, to_underlying(ClassMetaTableIndex::Parent));

    pushNumber(to_underlying(LuaDataType::GroundBrush));
    lua_rawseti(L, metaTableOffset, to_underlying(ClassMetaTableIndex::Type));

    pop(2);
}

void LuaState::registerMethod(const std::string &globalName, const std::string &methodName, lua_CFunction function)
{
    getGlobal(globalName);
    lua_pushcfunction(L, function);
    setFieldUnsafe(-2, methodName.c_str());

    pop();
}

void LuaState::pushCopy(int stackOffset)
{
    lua_pushvalue(L, stackOffset);
}

void LuaState::setMetaTable(int stackOffset)
{
    lua_setmetatable(L, stackOffset);
}

void LuaState::setMetaTable(lua_State *L, int32_t stackOffset, const std::string &name)
{
    if (stackOffset < 0)
    {
        luaL_getmetatable(L, name.c_str());

        // Pushing metatable onto the stack decreases offset by 1 (e.g. from -1 to -2)
        lua_setmetatable(L, stackOffset - 1);
    }
    else
    {
        // stackOffset is not affected if it is absolute (i.e. positive).
        luaL_getmetatable(L, name.c_str());
        lua_setmetatable(L, stackOffset);
    }
}

void LuaState::stackDump()
{
    stackDump(L);
}

void LuaState::stackDump(lua_State *L)
{
    int i;
    int top = topIndex(L);
    for (i = top; i >= 1; i--)
    { /* repeat for each level */
        LuaType luaType = type(L, i);
        switch (luaType)
        {
            case LuaType::String: /* strings */
                VME_LOG(toString(L, i));
                break;

            case LuaType::Bool: /* booleans */
                VME_LOG((toBool(L, i) ? "true" : "false"));
                break;

            case LuaType::Number: /* numbers */
                VME_LOG(toNumber(L, i));
                break;

            case LuaType::Table:
                VME_LOG("Table");
                break;

            default: /* other values */
                VME_LOG(typeName(L, luaType));
                break;
        }
    }

    VME_LOG("");
}