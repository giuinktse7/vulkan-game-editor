#include "lua_state.h"

#include "../debug.h"
#include "../util.h"
#include "lua_brush.h"

#if !defined LUA_VERSION_NUM || LUA_VERSION_NUM == 501
// From http://lua-users.org/wiki/CompatibilityWithLuaFive
static void luaL_setfuncs(lua_State *L, const luaL_Reg *l, int nup)
{
    luaL_checkstack(L, nup + 1, "too many upvalues");
    for (; l->name != NULL; l++)
    { /* fill the table with given functions */
        int i;
        lua_pushstring(L, l->name);
        for (i = 0; i < nup; i++) /* copy upvalues to the top */
            lua_pushvalue(L, -(nup + 1));
        lua_pushcclosure(L, l->func, nup); /* closure with those upvalues */
        lua_settable(L, -(nup + 3));
    }
    lua_pop(L, nup); /* remove upvalues */
}

bool lua_isinteger(lua_State *L, int index)
{
    return static_cast<LuaType>(lua_type(L, index)) == LuaType::Integer;
}
#endif

LuaState::LuaState()
{
}

LuaState::LuaState(lua_State *L)
    : L(L) {}

void LuaState::registerClass(lua_State *L, std::string className, lua_CFunction constructor)
{
    auto cName = className.c_str();

    // Create the global table
    {
        lua_newtable(L);
        int globalObject = lua_gettop(L);

        auto globalMetaTableName = (className + "__Type");

        luaL_newmetatable(L, globalMetaTableName.c_str());

        luaL_Reg methods[] = {
            {"__call", constructor},
            {nullptr, nullptr}};

        luaL_setfuncs(L, methods, 0);
        lua_setmetatable(L, globalObject);

        lua_setglobal(L, cName);
    }

    // Source: https://stackoverflow.com/questions/26970316/lua-userdata-array-access-and-methods
    // Indexing and member functions
    {
        luaL_newmetatable(L, cName);
        int metaTable = lua_gettop(L);

        auto indexFn = [](lua_State *L)
        {
            auto name = luaL_checkstring(L, lua_upvalueindex(1));
            return metaIndex(L, name);
        };

        luaL_Reg methods[] = {
            {"__index", indexFn},
            {nullptr, nullptr}};

        // Upvalue for indexFn
        lua_pushstring(L, cName);

        // Uses the UserData name as upvalue for __index function, hence '1' as last argument.
        luaL_setfuncs(L, methods, 1);

        lua_newtable(L);
        lua_setmetatable(L, metaTable);

        lua_pop(L, 1);

        // luaL_newlib(L, functions);
    }
}

void LuaState::registerFunction(lua_State *L, std::string typeName, std::string functionName, lua_CFunction function)
{
    // Meta table
    luaL_getmetatable(L, typeName.c_str());

    if (lua_isnil(L, -1))
    {
        std::string error = "There is no metatable for type '" + typeName + "'.";
        luaL_error(L, error.c_str());
    }

    luaL_Reg functions[] = {
        {functionName.c_str(), function},
        {nullptr, nullptr}};

    luaL_setfuncs(L, functions, 0);

    lua_pop(L, 1);
}

void LuaState::registerField(lua_State *L, std::string globalName, std::string fieldName, lua_CFunction function)
{
    // Meta table
    luaL_getmetatable(L, globalName.c_str());

    // Field table
    lua_getmetatable(L, -1);

    luaL_Reg functions[] = {
        {fieldName.c_str(), function},
        {nullptr, nullptr}};

    luaL_setfuncs(L, functions, 0);

    lua_pop(L, 2);
}

int LuaState::metaIndex(lua_State *L, std::string userDataId)
{
    auto id = userDataId.c_str();
    luaL_checkudata(L, 1, id);

    luaL_getmetatable(L, id);
    int metaOffset = lua_gettop(L);

    // Key to top of stack
    lua_pushvalue(L, 2);

    // Use key to index metatable
    lua_rawget(L, -2);

    if (lua_isnil(L, -1))
    {
        /* found no method, so get value from userdata. */
        auto index = luaL_checkstring(L, 2);

        lua_getmetatable(L, metaOffset);
        lua_getfield(L, -1, index);

        lua_replace(L, -2);

        lua_pushvalue(L, 1);

        lua_pcall(L, 1, 1, 0);
    };

    return 1;
}

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
    lua_getfield(L, stackOffset, name);
    return static_cast<LuaType>(lua_type(L, lua_gettop(L)));
}

LuaType LuaState::getField(lua_State *L, int stackOffset, const char *name)
{
    lua_getfield(L, stackOffset, name);
    return static_cast<LuaType>(lua_type(L, lua_gettop(L)));
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
    lua_getglobal(L, name);
    return static_cast<LuaType>(lua_type(L, lua_gettop(L)));
}

LuaType LuaState::getGlobal(const std::string &name)
{
    lua_getglobal(L, name.c_str());
    return static_cast<LuaType>(lua_type(L, lua_gettop(L)));
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

// void LuaState::registerClass(const std::string &className, const std::string &baseClass, lua_CFunction constructor)
// {
//     lua_newtable(L);
//     int methodsOffset = topIndex();

//     pushCopy(-1);
//     lua_setglobal(L, className.c_str());

//     lua_newtable(L);
//     int methodsMetaTableOffset = topIndex();

//     if (constructor)
//     {
//         lua_pushcfunction(L, constructor);
//         lua_setfield(L, methodsMetaTableOffset, "__call");
//     }

//     uint32_t parents = 0;
//     if (!baseClass.empty())
//     {
//         getGlobal(baseClass.c_str());
//         lua_rawgeti(L, -1, to_underlying(ClassMetaTableIndex::Parent));
//         parents = popIntUnsafe() + 1;
//         setFieldUnsafe(methodsMetaTableOffset, LuaMetaTable::Index);
//     }

//     setMetaTable(methodsOffset);

//     luaL_newmetatable(L, className.c_str());
//     int metaTableOffset = topIndex();

//     pushCopy(methodsOffset);
//     setFieldUnsafe(metaTableOffset, LuaMetaTable::MetaTable);

//     pushCopy(methodsOffset);
//     setFieldUnsafe(metaTableOffset, LuaMetaTable::Index);

//     size_t hash = std::hash<std::string>()(className);
//     pushNumber(hash);
//     lua_rawseti(L, metaTableOffset, to_underlying(ClassMetaTableIndex::Hash));

//     pushNumber(parents);
//     lua_rawseti(L, metaTableOffset, to_underlying(ClassMetaTableIndex::Parent));

//     pushNumber(to_underlying(LuaDataType::GroundBrush));
//     lua_rawseti(L, metaTableOffset, to_underlying(ClassMetaTableIndex::Type));

//     pop(2);
// }

// void LuaState::registerMethod(const std::string &globalName, const std::string &methodName, lua_CFunction function)
// {
//     getGlobal(globalName);
//     lua_pushcfunction(L, function);
//     setFieldUnsafe(-2, methodName.c_str());

//     pop();
// }

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

int LuaState::checkIntField(lua_State *L, const char *fieldName, int stackOffset)
{
    lua_getfield(L, stackOffset, fieldName);
    // if (!lua_isinteger(L, stackOffset))
    // {
    //     luaL_error(L, "Expected integer, but received %s.", toString(L, stackOffset).c_str());
    // }

    int result = lua_tointeger(L, stackOffset);
    lua_pop(L, 1);

    return result;
}

std::string LuaState::toString(int stackOffset)
{
    return lua_tostring(L, stackOffset);
}

std::string LuaState::toString(lua_State *L, int stackOffset)
{
    LuaType luaType = type(L, stackOffset);
    switch (luaType)
    {
        case LuaType::String: /* strings */
            return lua_tostring(L, stackOffset);
            break;

        case LuaType::Bool: /* booleans */
            return toBool(L, stackOffset) ? "true" : "false";
            break;

        case LuaType::Number: /* numbers */
            return std::to_string(toNumber(L, stackOffset));
            break;

        case LuaType::Table:
            return "Table";
            break;

        default: /* other values */
            return typeName(L, luaType);
            break;
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
    {
        VME_LOG(toString(L, i));
    }

    VME_LOG("");
}