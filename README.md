# HideParty - Windower 4 Addon

Hides native FFXI party list, alliance lists, and target bar UI elements via direct memory manipulation. The underlying game data remains fully functional — addons like XivParty can still read party/target information.

Ported from [Ashita v4 HideParty](https://github.com/AshitaXI/Ashita-v4beta) by atom0s.

## How It Works

FFXI's native UI elements (party list, target bar) are rendered as "primitive objects" in memory. HideParty scans `FFXiMain.dll` for known byte patterns (signatures) to locate these objects, then writes to their visibility flags every frame to keep them hidden.

Since Windower 4's Lua environment does not include LuaJIT FFI or expose memory read/write APIs, a small C helper DLL (`hideparty_mem.dll`) provides the memory operations. The DLL resolves LuaCore.dll's Lua C API at runtime via `GetProcAddress`, so it has **zero external dependencies** beyond `kernel32.dll`.
IF you are knew to C~ development, this previous line is important. Do not blind trust some chuckle fuck, like myself, on the internet with compiled code that you don't know anything about. All the source is here, It has nothing malicious in it, however, this should be said, as a learning point, IMO.

## Installation

1. Copy the `addon/HideParty.lua` file to `Windower/addons/HideParty/HideParty.lua`
2. Copy the `addon/hideparty_mem.dll` file to `Windower/plugins/libs/hideparty_mem.dll`
3. In game: `//lua load HideParty`

To auto-load on startup, add to your `Windower/scripts/init.txt`:
```
lua load HideParty
```

## Commands

| Command | Description |
|---------|-------------|
| `//hp hide party0` | Hide the main party list |
| `//hp hide party1` | Hide alliance party 1 |
| `//hp hide party2` | Hide alliance party 2 |
| `//hp hide target` | Hide the target bar |
| `//hp hide all` | Hide all elements |
| `//hp show party0` | Show the main party list |
| `//hp show all` | Show all elements |
| `//hp toggle party0` | Toggle an element's visibility |
| `//hp toggle all` | Toggle all elements |
| `//hp status` | Show current state and pointer addresses |
| `//hp help` | Show command list |

Settings are saved automatically and persist across sessions.

## Elements

| Name | Description |
|------|-------------|
| `party0` | Main party list (your party members) |
| `party1` | Alliance party 1 |
| `party2` | Alliance party 2 |
| `target` | Target bar (current target info) |

## Safety

- All memory reads/writes are wrapped in SEH (`__try/__except`) — invalid pointers won't crash the game
- All UI elements are automatically restored when the addon is unloaded or you log out
- Memory scanning pauses during zone transitions
- Signatures are scanned once at load time; if the FFXI client is updated and signatures break, the addon prints a warning and continues with whichever elements it found

## Building from Source

### Requirements
- Visual Studio 2022 (or any MSVC toolchain with x86 support)
- Windows SDK

### Steps

1. Open a **Windows Command Prompt** (not PowerShell)
2. Run `src/build.bat`

The build script:
- Sets up the MSVC x86 environment
- Generates a `LuaCore.lib` import library from the `.def` file
- Compiles `hideparty_mem.c` as a 32-bit DLL with static CRT (`/MT`)
- Copies the output to the Windower addon and plugins folders

### How the C Module Works

The DLL (`hideparty_mem.dll`) exports a single Lua C module entry point: `luaopen_hideparty_mem`. It provides these functions to Lua:

| Function | Description |
|----------|-------------|
| `find_module(name)` | Finds a loaded DLL by name, returns base address and size |
| `scan(signature)` | Scans the found module for a hex signature pattern (supports `??` wildcards) |
| `read_uint32(addr)` | Reads a 32-bit value from a memory address |
| `write_uint8(addr, val)` | Writes an 8-bit value to a memory address |
| `get_base()` | Returns the cached module base address |

The DLL has **no link-time dependency on LuaCore.dll**. Instead, it resolves the Lua C API functions at runtime from the already-loaded `LuaCore.dll` using `GetModuleHandleA` + `GetProcAddress`. This avoids DLL search path issues in FFXI's process.

## Signature Patterns

These byte patterns are scanned in `FFXiMain.dll`:

**Signature 1** (party0 + target):
```
66C78182000000????C7818C000000????????C781900000
```
- `party0` pointer at offset +0x19
- `target` pointer at offset +0x23

**Signature 2** (party1 + party2):
```
A1????????8B0D????????89442424A1????????33DB89
```
- `party1` pointer at offset +0x01
- `party2` pointer at offset +0x07

If FFXI is updated and these patterns break, they will need to be updated. Use a disassembler or signature scanner on the new `FFXiMain.dll` to find equivalent patterns.

## Credits

- **atom0s** — Original [Ashita v4 HideParty](https://github.com/AshitaXI/Ashita-v4beta) addon and signature research
- **Garretts** — Windower 4 port
