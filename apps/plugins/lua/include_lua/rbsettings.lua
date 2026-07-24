--[[ Lua rb settings reader / writer
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 William Wilgus
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
see https://forums.rockbox.org/index.php/topic,55820.0.html for an example
see rocklib.c for definitive list of supported settings
rb.global_settings()
rb.global_status()
rb.audio_next_track()
rb.audio_current_track()

NOTES:
Not all settings are accessible from lua, for instance you can read char_ptrs
but writing them is not supported since we cannot guarantee the lifetime once
lua terminates, or settigns containing unions since its not possible to discern
what the value is at compile time these items will be an opaque pointer to the
structure in memory but lua does not know how to decode it

If you are writing settings that you want to persist do not forget to call
rb.settings_save() though not calling it does NOT guarantee a changed setting
won't be written behind your back so be sure to save the old value
and restore it when you are finished and call rb.settings_save()
to ensure the old value gets saved

To Write stucts and arrays it is recommended to first make a call to
newval = settings_read('global_settings', 'setting.foo', 'system')
and make changes to the returned table
    newval.foo = bar
followed by
    setting_writesettings_read('global_settings', 'rb.foo', 'system', newval)
with the altered table.
]]

rb.settings = rb.settings or {}

local var = {offset = 1, size = 2, type = 3, fields = 3}

local function bytesLE_n(str)
    str = str or ""
    local tbyte={str:byte(1, -1)}
    local bpos, num = 1, 0
    for k = 1,#tbyte do -- (k = #t, 1, -1 for BE)
        num = num + tbyte[k] * bpos
        bpos = bpos * 256 --1<<8
    end
    return num
end

local function get_var_fields(s_var)
    -- converts member string into table
    -- var = {offset, size, "type"}
    s_var = s_var or ""
    local o, s, t = string.match(s_var, "(0x%x+),%s*(%d+),%s*(.+)")
    local tvar = {o, s, t}

    return #tvar == var.fields and tvar or nil
end

local function format_val(val, var_type)
    local ret, num

    if var_type == nil then
        return nil
    elseif var_type == "str" then
        -- stop at first null byte, return nil if str doesn't exist
        return val and string.match(val, "^%Z+") or nil
    end

    num = bytesLE_n(val)
    if string.find(var_type, "^b") then
        if(num <= 0) then
            ret = false
        else
            ret = true
        end
    elseif string.find(var_type, "^u_[cil]") then
        -- Lua integers are signed so we need to do a bit of extra processing
        ret = (string.format("%u", num))
    else
        ret = num
    end

    return ret
end

--[[ ex. dump_struct(rb.global_settings,
          rb.system.replaygain_settings, 0, [0x2c, 12, s_replaygain_settings])]]

local function dump_struct(t_settings, t_struct, n_elems, t_var)
    --Internal function dumps structs
    local tdata = {}

    local function struct_get_elem(v, elem_offset)
        local val, offset, tvar1
        tvar1 = get_var_fields(v)
        offset = t_var[var.offset] + tvar1[var.offset] + elem_offset
        val = t_settings(offset, tvar1[var.size])
        return format_val(val, tvar1[var.type])
    end

    if n_elems > 0 then
        -- Array of structs, struct[elems];
        local elemsize = (t_var[var.size] / n_elems)
        for i = 0, n_elems - 1 do
            tdata[i] = tdata[i] or {}
            for k1, v1 in pairs(t_struct) do
                tdata[i][k1] = struct_get_elem(v1, (elemsize * i))
            end
        end
    else
        -- single struct, struct;
        for k1, v1 in pairs(t_struct) do
            tdata[k1] = struct_get_elem(v1, 0)
        end
    end
    return tdata
end

--[[ ex. write_struct(rb.global_settings,
            rb.system.eq_band_setting, 10, [ 0x54, 120, s_eq_band_setting[10] ])]]
local function write_struct(t_settings, t_struct, n_elems, t_var, tdata)
    --Internal function writes structs
   local function struct_write_elem(v, elem_offset, newval)
        local offset, tvar1
        tvar1 = get_var_fields(v)
        offset = t_var[var.offset] + tvar1[var.offset] + elem_offset
        return t_settings(offset, tvar1[var.size], newval) == 1
    end

    if n_elems > 0 then
        -- Array of structs, struct[elems];
        local elemsize = (t_var[var.size] / n_elems)
        for i = 0, n_elems - 1 do
            tdata[i] = tdata[i] or {}
            for k1, v1 in pairs(t_struct) do
                --tdata[i][k1] = struct_get_elem(v1, (elemsize * i))
                if tdata[i][k1] ~= nil then
                    if not struct_write_elem(v1, (elemsize * i), tdata[i][k1]) then
                        return false
                    end
                end
            end
        end
    else
        -- single struct, struct;
        for k1, v1 in pairs(t_struct) do
            if tdata[k1] ~= nil then
                if not struct_write_elem(v1, 0, tdata[k1]) then
                    return false
                end
            end
        end
    end
    return true
end

local function get_array_elems(var_type)
    --extract the number of elements, returns 0 if not found
    local elems = string.match(var_type,".*%[(%d+)%]")
    return tonumber(elems) or 0
end

local function get_struct_name(var_type)
    --extract the name of a struct, returns nil if not found
    return string.match(var_type,"^s_([^%[%]%s]+)")
end

function rb.settings.read(s_settings, s_var, s_groupname)
    local data, val
    local tvar = get_var_fields(s_var)
    if tvar == nil then return nil end

    local elems = get_array_elems(tvar[var.type])
    local structname = get_struct_name(tvar[var.type])

    local tsettings = rb[s_settings]
    if not tsettings then error(s_settings .. " does not exist") end

    if structname and rb[s_groupname] then
        return dump_struct(tsettings, rb[s_groupname][structname], elems, tvar)
    end

    local voffset, vsize, vtype = tvar[var.offset], tvar[var.size], tvar[var.type]
    if elems > 0 then
        -- Arrays of values, val[elems];
        data = {}
        local elemsize = (vsize / elems)

        for i = 0, elems - 1 do
            val = tsettings(voffset + (elemsize * i), elemsize)
            data[i] = format_val(val, vtype)
        end
    else
        -- Single value, val;
        if vtype == "ptr_char" then -- (**char)
            vtype = "str"
            val = tsettings(voffset, vsize, nil, true)
        else
            val = tsettings(voffset, vsize)
        end
        data = format_val(val, vtype)
    end
    return data
end

function rb.settings.write(s_settings, s_var, s_groupname, newval)
    local data, val
    local res = 0
    local tvar = get_var_fields(s_var)
    if tvar == nil then return nil end

    local elems = get_array_elems(tvar[var.type])
    local structname = get_struct_name(tvar[var.type])

    local tsettings = rb[s_settings]
    if not tsettings then error(s_settings .. " does not exist") end
    if not newval then error(s_settings .. " nothing to write") end

    if structname and rb[s_groupname] then
        -- rb.splash(300, "struct " ..  structname .. " - " .. s_groupname)
        if type (newval) ~= "table" then
            error("newval table expected got " .. type(newval))
        end
        if not write_struct(tsettings, rb[s_groupname][structname], elems, tvar, newval) then
            error(s_groupname .. " writing failed")
        end
        return 1
    end

    local voffset, vsize, vtype = tvar[var.offset], tvar[var.size], tvar[var.type]
    if elems > 0 then
        if type (newval) ~= "table" then
            error("newval table expected got " .. type(newval))
        end
        -- Arrays of values, val[elems];
        local elemsize = (vsize / elems)

        for i = 0, elems - 1 do
            if newval[i] ~= nil then
                if tsettings(voffset + (elemsize * i), elemsize, newval[i]) ~= 1 then
                    res = 0
                    break
                end
            end
        end
    else
        -- Single value, val;
        if vtype == "ptr_char" then -- (**char)
            error(s_var .. " is a string pointer")
        else
            res = tsettings(voffset, vsize, newval)
        end
    end
    return res
end

function rb.settings.dump(s_settings, s_groupname, s_structname, t_output, fn_filter)
    t_output = t_output or {}
    fn_filter = fn_filter or function(s,k) return true end
    local tgroup = rb[s_groupname]
    s_structname = s_structname or s_settings
    for k, v in pairs(tgroup[s_structname]) do
        if fn_filter(s_structname, k) then
            t_output[k] = rb.settings.read(s_settings, v, s_groupname)
        end
    end
    return t_output
end

return true
