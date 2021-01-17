
#include "lua_test.h"

#include <lua.hpp>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <initializer_list>
#include <iostream>
#include <string>

#include "./debug.h"

static void stackDump(lua_State *L)
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

void luaError(lua_State *L, const std::string &error)
{
    VME_LOG_ERROR(error);
    lua_close(L);
    exit(EXIT_FAILURE);
}

template <typename T>
void func(T t)
{
    std::cout << t << std::endl;
}

template <typename T, typename... Args>
void func(T t, Args... args) // recursive variadic function
{
    std::cout << t << std::endl;

    func(args...);
}

template <class T>
void func2(std::initializer_list<T> list)
{
    for (auto elem : list)
    {
        std::cout << elem << std::endl;
    }
}

void testLua()
{

    std::string str1("Hello"), str2("world");

    func(1, 2.5, 'a', str1);

    func2({10, 20, 30, 40});
    func2({str1, str2});
    return;

    // sol::state lua;
    // // open some common libraries
    // lua.open_libraries(sol::lib::base, sol::lib::package);
    // lua.script("print('bark bark bark!')");

    // std::cout << std::endl;
    lua_State *L = luaL_newstate();
    if (!L)
    {
        VME_LOG("No memory for lua.");
        return;
    }

    luaL_openlibs(L);

    if (luaL_dofile(L, "test.lua"))
    {
        VME_LOG("[Error - Lua load] " << lua_tostring(L, -1));
        lua_close(L);
        return;
    }

    // lua_pushglobaltable(L); // Get global table
    // lua_getglobal(L, "x");
    // if (!lua_isnumber(L, 1))
    // {
    //     VME_LOG("rip 1");
    //     return;
    // }

    lua_getglobal(L, "width");
    lua_getglobal(L, "height");

    if (!lua_isnumber(L, -1))
    {
        std::string error = "cannot run configuration file: ";
        error += lua_tostring(L, -1);
        luaError(L, error);
    }

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