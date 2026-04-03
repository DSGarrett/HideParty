/*
 * hideparty_mem.c - Lua C module for memory operations
 * Provides signature scanning and memory read/write for HideParty addon.
 *
 * This version resolves Lua C API functions at runtime from the already-loaded
 * LuaCore.dll, so it has NO import dependency on LuaCore.dll.
 */

#include <windows.h>
#include <psapi.h>

/* ------------------------------------------------------------------ */
/* Lua types and function pointers (resolved at runtime)               */
/* ------------------------------------------------------------------ */

typedef struct lua_State lua_State;
typedef int (*lua_CFunction)(lua_State *L);
typedef double lua_Number;
typedef ptrdiff_t lua_Integer;

/* Function pointer types for the Lua C API functions we need */
typedef lua_Integer (*fn_lua_tointeger)(lua_State*, int);
typedef const char* (*fn_lua_tolstring)(lua_State*, int, size_t*);
typedef void (*fn_lua_pushnil)(lua_State*);
typedef void (*fn_lua_pushinteger)(lua_State*, lua_Integer);
typedef void (*fn_lua_pushstring)(lua_State*, const char*);
typedef void (*fn_lua_pushcclosure)(lua_State*, lua_CFunction, int);
typedef void (*fn_lua_createtable)(lua_State*, int, int);
typedef void (*fn_lua_settable)(lua_State*, int);

/* Global function pointers */
static fn_lua_tointeger   p_lua_tointeger   = NULL;
static fn_lua_tolstring   p_lua_tolstring   = NULL;
static fn_lua_pushnil     p_lua_pushnil     = NULL;
static fn_lua_pushinteger p_lua_pushinteger = NULL;
static fn_lua_pushstring  p_lua_pushstring  = NULL;
static fn_lua_pushcclosure p_lua_pushcclosure = NULL;
static fn_lua_createtable p_lua_createtable = NULL;
static fn_lua_settable    p_lua_settable    = NULL;

/* Convenience macros */
#define lua_tointeger(L,i)    p_lua_tointeger(L,i)
#define lua_tostring(L,i)     p_lua_tolstring(L,i,NULL)
#define lua_pushnil(L)        p_lua_pushnil(L)
#define lua_pushinteger(L,n)  p_lua_pushinteger(L,n)
#define lua_pushstring(L,s)   p_lua_pushstring(L,s)
#define lua_pushcfunction(L,f) p_lua_pushcclosure(L,f,0)
#define lua_newtable(L)       p_lua_createtable(L,0,0)
#define lua_settable(L,i)     p_lua_settable(L,i)

static int resolve_lua_api(void)
{
    /* Find LuaCore.dll which is already loaded in this process */
    HMODULE hLua = GetModuleHandleA("LuaCore.dll");
    if (!hLua) return 0;

    p_lua_tointeger   = (fn_lua_tointeger)  GetProcAddress(hLua, "lua_tointeger");
    p_lua_tolstring   = (fn_lua_tolstring)  GetProcAddress(hLua, "lua_tolstring");
    p_lua_pushnil     = (fn_lua_pushnil)    GetProcAddress(hLua, "lua_pushnil");
    p_lua_pushinteger = (fn_lua_pushinteger)GetProcAddress(hLua, "lua_pushinteger");
    p_lua_pushstring  = (fn_lua_pushstring) GetProcAddress(hLua, "lua_pushstring");
    p_lua_pushcclosure= (fn_lua_pushcclosure)GetProcAddress(hLua, "lua_pushcclosure");
    p_lua_createtable = (fn_lua_createtable)GetProcAddress(hLua, "lua_createtable");
    p_lua_settable    = (fn_lua_settable)   GetProcAddress(hLua, "lua_settable");

    return (p_lua_tointeger && p_lua_tolstring && p_lua_pushnil &&
            p_lua_pushinteger && p_lua_pushstring && p_lua_pushcclosure &&
            p_lua_createtable && p_lua_settable) ? 1 : 0;
}

/* ------------------------------------------------------------------ */
/* Memory helpers                                                      */
/* ------------------------------------------------------------------ */

static BYTE* g_module_base = NULL;
static DWORD g_module_size = 0;

static int parse_signature(const char* sig, BYTE* out_bytes, BOOL* out_mask, int max_len)
{
    int len = 0;
    while (*sig && len < max_len) {
        if (sig[0] == '?' && sig[1] == '?') {
            out_bytes[len] = 0;
            out_mask[len] = FALSE;
            sig += 2;
        } else {
            char hex[3] = { sig[0], sig[1], 0 };
            out_bytes[len] = (BYTE)strtoul(hex, NULL, 16);
            out_mask[len] = TRUE;
            sig += 2;
        }
        len++;
    }
    return len;
}

static int scan_for_signature(BYTE* base, DWORD size, const char* sig_str)
{
    BYTE pattern[256];
    BOOL mask[256];
    int pat_len;
    DWORD i;
    int j;

    pat_len = parse_signature(sig_str, pattern, mask, 256);
    if (pat_len == 0 || (DWORD)pat_len > size) return -1;

    for (i = 0; i <= size - (DWORD)pat_len; i++) {
        BOOL found = TRUE;
        for (j = 0; j < pat_len; j++) {
            if (mask[j] && base[i + j] != pattern[j]) {
                found = FALSE;
                break;
            }
        }
        if (found) return (int)i;
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/* Lua-exposed functions                                               */
/* ------------------------------------------------------------------ */

/* hideparty_mem.find_module(name) -> base_addr, size  or nil */
static int l_find_module(lua_State* L)
{
    const char* name = lua_tostring(L, 1);
    HMODULE hmod;
    MODULEINFO mi;

    if (!name) { lua_pushnil(L); return 1; }

    hmod = GetModuleHandleA(name);
    if (!hmod) { lua_pushnil(L); return 1; }

    if (!K32GetModuleInformation(GetCurrentProcess(), hmod, &mi, sizeof(mi))) {
        lua_pushnil(L);
        return 1;
    }

    g_module_base = (BYTE*)mi.lpBaseOfDll;
    g_module_size = mi.SizeOfImage;

    lua_pushinteger(L, (lua_Integer)(DWORD)mi.lpBaseOfDll);
    lua_pushinteger(L, (lua_Integer)mi.SizeOfImage);
    return 2;
}

/* hideparty_mem.scan(signature_hex_string) -> offset or nil */
static int l_scan(lua_State* L)
{
    const char* sig;
    int offset;

    if (!g_module_base || g_module_size == 0) { lua_pushnil(L); return 1; }

    sig = lua_tostring(L, 1);
    if (!sig) { lua_pushnil(L); return 1; }

    offset = scan_for_signature(g_module_base, g_module_size, sig);
    if (offset < 0) { lua_pushnil(L); return 1; }

    lua_pushinteger(L, (lua_Integer)offset);
    return 1;
}

/* hideparty_mem.read_uint32(address) -> value */
static int l_read_uint32(lua_State* L)
{
    DWORD addr = (DWORD)lua_tointeger(L, 1);
    DWORD val = 0;

    if (addr != 0) {
        __try { val = *(DWORD*)addr; }
        __except(EXCEPTION_EXECUTE_HANDLER) { val = 0; }
    }

    lua_pushinteger(L, (lua_Integer)val);
    return 1;
}

/* hideparty_mem.write_uint8(address, value) */
static int l_write_uint8(lua_State* L)
{
    DWORD addr = (DWORD)lua_tointeger(L, 1);
    BYTE val = (BYTE)lua_tointeger(L, 2);

    if (addr != 0) {
        __try { *(BYTE*)addr = val; }
        __except(EXCEPTION_EXECUTE_HANDLER) { /* silently fail */ }
    }

    return 0;
}

/* hideparty_mem.get_base() -> base_addr or 0 */
static int l_get_base(lua_State* L)
{
    lua_pushinteger(L, (lua_Integer)(DWORD)g_module_base);
    return 1;
}

/* ------------------------------------------------------------------ */
/* Module entry point                                                  */
/* ------------------------------------------------------------------ */

__declspec(dllexport) int luaopen_hideparty_mem(lua_State* L)
{
    typedef struct { const char* name; lua_CFunction func; } RegEntry;
    static const RegEntry funcs[] = {
        { "find_module", l_find_module },
        { "scan",        l_scan },
        { "read_uint32", l_read_uint32 },
        { "write_uint8", l_write_uint8 },
        { "get_base",    l_get_base },
        { NULL, NULL }
    };
    int i;

    /* Resolve Lua API from already-loaded LuaCore.dll */
    if (!resolve_lua_api()) {
        /* Can't even push an error without the API. Just return 0. */
        return 0;
    }

    lua_newtable(L);
    for (i = 0; funcs[i].name != NULL; i++) {
        lua_pushstring(L, funcs[i].name);
        lua_pushcfunction(L, funcs[i].func);
        lua_settable(L, -3);
    }

    return 1;
}
