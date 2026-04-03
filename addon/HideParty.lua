--[[
    HideParty - Windower 4 Addon
    Hides native FFXI party list and target bar UI elements.
    Ported from Ashita v4 HideParty by atom0s.
    Uses a C module (hideparty_mem.dll) for memory operations.

    Commands:
        //hp hide [party0|party1|party2|target|all]
        //hp show [party0|party1|party2|target|all]
        //hp toggle [party0|party1|party2|target|all]
        //hp status
        //hp help
]]

_addon.name    = 'HideParty'
_addon.author  = 'Garretts'
_addon.version = '1.0.0'
_addon.commands = {'hideparty', 'hp'}

config = require('config')

---------------------------------------------------------------------------
-- Load C memory module
---------------------------------------------------------------------------

-- Try loading the C module via require (searches package.cpath including plugins/libs/)
local mem_ok, mem = pcall(require, 'hideparty_mem')
if not mem_ok or not mem then
    windower.add_to_chat(167, 'HideParty: ERROR - Could not load hideparty_mem: ' .. tostring(mem))
    windower.add_to_chat(167, 'HideParty: package.cpath = ' .. tostring(package.cpath))
    return
end

windower.add_to_chat(207, 'HideParty: Memory module loaded.')

---------------------------------------------------------------------------
-- Settings
---------------------------------------------------------------------------

local defaults = {
    party0 = false,
    party1 = false,
    party2 = false,
    target = false,
}

local settings = config.load(defaults)

---------------------------------------------------------------------------
-- State
---------------------------------------------------------------------------

local initialized = false
local is_zoning = false

local ptrs = {
    party0 = 0,
    party1 = 0,
    party2 = 0,
    target = 0,
}

local ELEMENT_NAMES = {'party0', 'party1', 'party2', 'target'}

---------------------------------------------------------------------------
-- Visibility control
---------------------------------------------------------------------------

local function set_visibility(base_ptr_addr, visible)
    if base_ptr_addr == 0 then return end

    local ptr1 = mem.read_uint32(base_ptr_addr)
    if ptr1 == 0 then return end

    local ptr2 = mem.read_uint32(ptr1 + 0x08)
    if ptr2 == 0 then return end

    local val = visible and 1 or 0
    mem.write_uint8(ptr2 + 0x69, val)
    mem.write_uint8(ptr2 + 0x6A, val)
end

local function show_element(name)
    if ptrs[name] and ptrs[name] ~= 0 then
        set_visibility(ptrs[name], true)
    end
    settings[name] = false
    settings:save()
end

local function hide_element(name)
    settings[name] = true
    settings:save()
end

local function show_all()
    for _, name in ipairs(ELEMENT_NAMES) do
        show_element(name)
    end
end

local function hide_all()
    for _, name in ipairs(ELEMENT_NAMES) do
        hide_element(name)
    end
end

local function toggle_element(name)
    if settings[name] then
        show_element(name)
    else
        hide_element(name)
    end
end

---------------------------------------------------------------------------
-- Initialization
---------------------------------------------------------------------------

local function initialize()
    local base, size = mem.find_module('FFXiMain.dll')
    if not base then
        windower.add_to_chat(167, 'HideParty: ERROR - Could not find FFXiMain.dll in memory.')
        return false
    end

    windower.add_to_chat(207, string.format('HideParty: Scanning FFXiMain.dll (base: 0x%X, size: %d bytes)...', base, size))

    -- Signature 1: party0 and target
    local sig1 = mem.scan('66C78182000000????C7818C000000????????C781900000')
    if sig1 then
        ptrs.party0 = mem.read_uint32(base + sig1 + 0x19)
        ptrs.target = mem.read_uint32(base + sig1 + 0x23)
        windower.add_to_chat(207, string.format('  Sig1 found: party0=0x%X, target=0x%X', ptrs.party0, ptrs.target))
    else
        windower.add_to_chat(167, '  HideParty: WARNING - Signature 1 not found (party0/target).')
    end

    -- Signature 2: party1 and party2
    local sig2 = mem.scan('A1????????8B0D????????89442424A1????????33DB89')
    if sig2 then
        ptrs.party1 = mem.read_uint32(base + sig2 + 0x01)
        ptrs.party2 = mem.read_uint32(base + sig2 + 0x07)
        windower.add_to_chat(207, string.format('  Sig2 found: party1=0x%X, party2=0x%X', ptrs.party1, ptrs.party2))
    else
        windower.add_to_chat(167, '  HideParty: WARNING - Signature 2 not found (party1/party2).')
    end

    local found_any = (sig1 ~= nil) or (sig2 ~= nil)
    if found_any then
        windower.add_to_chat(207, 'HideParty: Initialized successfully.')
    else
        windower.add_to_chat(167, 'HideParty: ERROR - No signatures found. FFXI version may be incompatible.')
    end

    return found_any
end

---------------------------------------------------------------------------
-- Events
---------------------------------------------------------------------------

windower.register_event('load', function()
    initialized = initialize()
end)

windower.register_event('prerender', function()
    if not initialized or is_zoning then return end

    for _, name in ipairs(ELEMENT_NAMES) do
        if settings[name] and ptrs[name] ~= 0 then
            set_visibility(ptrs[name], false)
        end
    end
end)

windower.register_event('incoming chunk', function(id, data)
    if id == 0x0B then
        is_zoning = true
    elseif id == 0x0A then
        is_zoning = false
    end
end)

windower.register_event('unload', function()
    if not initialized then return end
    for _, name in ipairs(ELEMENT_NAMES) do
        if ptrs[name] ~= 0 then
            set_visibility(ptrs[name], true)
        end
    end
end)

windower.register_event('logout', function()
    if not initialized then return end
    for _, name in ipairs(ELEMENT_NAMES) do
        if ptrs[name] ~= 0 then
            set_visibility(ptrs[name], true)
        end
    end
end)

---------------------------------------------------------------------------
-- Commands
---------------------------------------------------------------------------

local function print_status()
    windower.add_to_chat(207, 'HideParty Status:')
    for _, name in ipairs(ELEMENT_NAMES) do
        local state = settings[name] and 'HIDDEN' or 'shown'
        local ptr_state = ptrs[name] ~= 0 and string.format('0x%X', ptrs[name]) or 'not found'
        windower.add_to_chat(207, string.format('  %-8s: %-6s  (ptr: %s)', name, state, ptr_state))
    end
end

local function print_help()
    windower.add_to_chat(207, 'HideParty Commands:')
    windower.add_to_chat(207, '  //hp hide [party0|party1|party2|target|all]')
    windower.add_to_chat(207, '  //hp show [party0|party1|party2|target|all]')
    windower.add_to_chat(207, '  //hp toggle [party0|party1|party2|target|all]')
    windower.add_to_chat(207, '  //hp status')
    windower.add_to_chat(207, '  //hp help')
end

local function is_valid_element(name)
    for _, n in ipairs(ELEMENT_NAMES) do
        if n == name then return true end
    end
    return false
end

windower.register_event('addon command', function(command, ...)
    local args = {...}
    command = command and command:lower() or 'help'
    local element = args[1] and args[1]:lower() or 'all'

    if command == 'hide' then
        if element == 'all' then
            hide_all()
            windower.add_to_chat(207, 'HideParty: All elements hidden.')
        elseif is_valid_element(element) then
            hide_element(element)
            windower.add_to_chat(207, 'HideParty: ' .. element .. ' hidden.')
        else
            windower.add_to_chat(167, 'HideParty: Unknown element "' .. element .. '". Use: party0, party1, party2, target, all')
        end

    elseif command == 'show' then
        if element == 'all' then
            show_all()
            windower.add_to_chat(207, 'HideParty: All elements shown.')
        elseif is_valid_element(element) then
            show_element(element)
            windower.add_to_chat(207, 'HideParty: ' .. element .. ' shown.')
        else
            windower.add_to_chat(167, 'HideParty: Unknown element "' .. element .. '". Use: party0, party1, party2, target, all')
        end

    elseif command == 'toggle' then
        if element == 'all' then
            for _, name in ipairs(ELEMENT_NAMES) do
                toggle_element(name)
            end
            windower.add_to_chat(207, 'HideParty: All elements toggled.')
        elseif is_valid_element(element) then
            toggle_element(element)
            local state = settings[element] and 'hidden' or 'shown'
            windower.add_to_chat(207, 'HideParty: ' .. element .. ' ' .. state .. '.')
        else
            windower.add_to_chat(167, 'HideParty: Unknown element "' .. element .. '". Use: party0, party1, party2, target, all')
        end

    elseif command == 'status' then
        print_status()

    else
        print_help()
    end
end)
