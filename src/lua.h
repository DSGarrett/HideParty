/* Minimal Lua 5.1 C API header for our module */
#ifndef LUA_H
#define LUA_H

#include <stddef.h>

typedef struct lua_State lua_State;
typedef int (*lua_CFunction)(lua_State *L);
typedef double lua_Number;
typedef ptrdiff_t lua_Integer;

#define LUA_REGISTRYINDEX (-10000)
#define LUA_GLOBALSINDEX  (-10002)

/* State */
extern void lua_close(lua_State *L);

/* Stack */
extern int  lua_gettop(lua_State *L);
extern void lua_settop(lua_State *L, int idx);
extern void lua_pushvalue(lua_State *L, int idx);
extern void lua_remove(lua_State *L, int idx);
extern void lua_insert(lua_State *L, int idx);
extern void lua_replace(lua_State *L, int idx);

/* Type checks */
extern int lua_type(lua_State *L, int idx);
extern int lua_isnumber(lua_State *L, int idx);
extern int lua_isstring(lua_State *L, int idx);

/* Get values */
extern lua_Number  lua_tonumber(lua_State *L, int idx);
extern lua_Integer lua_tointeger(lua_State *L, int idx);
extern int         lua_toboolean(lua_State *L, int idx);
extern const char *lua_tolstring(lua_State *L, int idx, size_t *len);

/* Push values */
extern void lua_pushnil(lua_State *L);
extern void lua_pushnumber(lua_State *L, lua_Number n);
extern void lua_pushinteger(lua_State *L, lua_Integer n);
extern void lua_pushlstring(lua_State *L, const char *s, size_t len);
extern void lua_pushstring(lua_State *L, const char *s);
extern void lua_pushboolean(lua_State *L, int b);
extern void lua_pushcclosure(lua_State *L, lua_CFunction fn, int n);

#define lua_pushcfunction(L,f) lua_pushcclosure(L, (f), 0)

/* Table */
extern void lua_createtable(lua_State *L, int narr, int nrec);
extern void lua_settable(lua_State *L, int idx);
extern void lua_setfield(lua_State *L, int idx, const char *k);
extern void lua_rawset(lua_State *L, int idx);
extern void lua_rawseti(lua_State *L, int idx, int n);

#define lua_newtable(L) lua_createtable(L, 0, 0)

/* Aux library */
extern int luaL_error(lua_State *L, const char *fmt, ...);
extern void luaL_register(lua_State *L, const char *libname, const void *l);

/* Convenience */
#define lua_pop(L,n) lua_settop(L, -(n)-1)
#define lua_tostring(L,i) lua_tolstring(L, (i), NULL)

#endif
