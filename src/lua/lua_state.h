#pragma once

#if __has_include("luajit/lua.hpp")
#include <luajit/lua.hpp>
#else
#include <lua.hpp>
#endif

#include <optional>
#include <stdint.h>
#include <string>
#include <type_traits>

#include "../logger.h"

#ifndef EXIT_LUA_ERROR
#define EXIT_LUA_ERROR(errorString) errorAbort(FILE_AND_LINE_STR + errorString)
#endif

#ifndef SHOW_LUA_ERROR
#define SHOW_LUA_ERROR(errorString) VME_LOG_ERROR(FILE_AND_LINE_STR + errorString)
#endif

class LuaScriptInterface;
struct lua_State;

enum class LuaDataType
{
    GroundBrush
};

enum class LuaType
{
    // Types matching lua.h types
    None = LUA_TNONE,
    Nil = LUA_TNIL,
    Bool = LUA_TBOOLEAN,
    LightUserData = LUA_TLIGHTUSERDATA,
    Number = LUA_TNUMBER,
    String = LUA_TSTRING,
    Table = LUA_TTABLE,
    Function = LUA_TFUNCTION,
    UserData = LUA_TUSERDATA,
    Thread = LUA_TTHREAD,

    // Custom types
    Integer = 100 // Arbitrary offset
};

enum class ClassMetaTableIndex
{
    // 0 unused
    Parent = 1,
    Hash = 2,
    Type = 3
};

struct LuaMetaTable
{
    static auto constexpr Index = "__index";
    static auto constexpr NewIndex = "__newindex";
    static auto constexpr MetaTable = "__metatable";
};

class LuaState
{
  public:
    /* Begin static */
    static void registerClass(lua_State *L, std::string className, lua_CFunction constructor);
    static void registerFunction(lua_State *L, std::string globalName, std::string functionName, lua_CFunction function);
    static void registerField(lua_State *L, std::string globalName, std::string fieldName, lua_CFunction function);

    template <typename T>
    static typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, T>::type
    getNumber(int32_t arg);

    template <typename T>
    static T getNumber(int32_t arg, T defaultValue);

    template <class T>
    static void pushUserData(lua_State *L, T *value);

    template <class T>
    static T *getUserdata(lua_State *L, int32_t arg);

    template <class T>
    static T *checkUserData(lua_State *L, int32_t arg, const char *typeName);

    static void setMetaTable(lua_State *L, int32_t stackOffset, const std::string &name);

    static LuaType getField(lua_State *L, int stackOffset, const char *name);
    /* End static */

    LuaState(lua_State *L);

    bool initialize();

    bool doFile(std::string file);

    bool registerTable(const std::string &tableName);

    // void registerClass(const std::string &className, const std::string &baseClass, lua_CFunction constructor = nullptr);
    // void registerMethod(const std::string &globalName, const std::string &methodName, lua_CFunction function);

    void setMetaTable(int stackOffset);

    std::string toString(int stackOffset);
    static std::string toString(lua_State *L, int stackOffset = -1);
    lua_Number toNumber(int stackOffset);
    static lua_Number toNumber(lua_State *L, int stackOffset);
    bool toBool(int stackOffset);
    static bool toBool(lua_State *L, int stackOffset);

    void pop(int amount = 1);

    int popInt(bool *ok);
    int popIntUnsafe();

    std::optional<std::string> popString();
    std::string popStringUnsafe();

    void close();

    static LuaType type(lua_State *L, int stackOffset);
    LuaType type(int stackOffset) const;
    std::string typeName(LuaType type) const;
    static std::string typeName(lua_State *L, LuaType type);

    int topIndex() const;
    static int topIndex(lua_State *L);

    void stackDump();
    static void stackDump(lua_State *L);

    bool isTable(int stackOffset) const;
    bool isNumber(int stackOffset) const;
    bool isString(int stackOffset) const;

    static int checkIntField(lua_State *L, const char *fieldName, int stackOffset = -1);

    void pushNil();
    void pushNumber(lua_Number number);
    void pushCopy(int stackOffset);

    bool next(int stackOffset);

    LuaType getField(int stackOffset, const char *name);
    bool getField(int stackOffset, const char *name, LuaType requiredType);

    void setFieldUnsafe(int stackOffset, const char *name);

    // Read a value, leaving the stack unchanged after the function returns.
    std::optional<std::string> readTableString(int tableStackOffset, const char *name);

    LuaType getGlobal(const char *name);
    LuaType getGlobal(const std::string &name);

    lua_State *L;

  private:
    friend class LuaScriptInterface;
    LuaState();

    static int metaIndex(lua_State *L, std::string userDataId);
};

template <class T>
void LuaState::pushUserData(lua_State *L, T *value)
{
    T **userdata = static_cast<T **>(lua_newuserdata(L, sizeof(T *)));
    *userdata = value;
}

template <class T>
T *LuaState::getUserdata(lua_State *L, int32_t arg)
{
    T **userdata = static_cast<T **>(lua_touserdata(L, arg));
    return userdata ? *userdata : nullptr;
}

template <class T>
T *LuaState::checkUserData(lua_State *L, int32_t arg, const char *typeName)
{
    T **userdata = static_cast<T **>(luaL_checkudata(L, arg, typeName));
    return userdata ? *userdata : nullptr;
}

template <typename T>
typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, T>::type
LuaState::getNumber(int32_t arg)
{
    return static_cast<T>(lua_tonumber(L, arg));
}

template <typename T>
T LuaState::getNumber(int32_t arg, T defaultValue)
{
    const auto parameters = lua_gettop(L);
    if (parameters == 0 || arg > parameters)
    {
        return defaultValue;
    }
    return getNumber<T>(L, arg);
}
