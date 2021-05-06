
--RB LUA show all global variables; BILGUS
if not ... then --if executed directly this is nil
    require "actions"
    require "audio"
    require "buttons"
    require "color"
    require "draw"
    require "draw_floodfill"
    require "draw_poly"
    require "draw_text"

    require "image"
    require "image_save"

    require "lcd"
    require "math_ex"
    require "pcm"
    require "playlist"
    require "print"
    --require "settings" --uses a lot of memory
    require "sound"
end
collectgarbage("collect")

local sDumpFile = "/rb-lua_functions.txt"
local filehandle

local function a2m_m2a(addr_member)
    --turns members into addresses; addresses back into members
    return addr_member
end

local function dtTag(sType)
--convert named type; 'number'.. to short type '[n]...'
--if '?' supplied print out datatype key; number = [n]...
    local retType = "?"
    local typ = {
                ["nil"] = "nil",
                ["boolean"]  = "b",
                ["number"] = "n",
                ["string"] = "s",
                ["userdata"] = "u",
                ["function"] = "f",
                ["thread"] = "thr",
                ["table"] = "t"
                }
    if sType == "?" then retType = "Datatypes: " end
    for k,v in pairs(typ) do
        if sType == k then
            retType = v break
        elseif (sType == "?") then
            retType = retType .. "  [" ..v.. "] = " .. k
        end
    end
    return " [" ..retType.. "] "
end

local function tableByName(tName)
    --find the longest match possible to an actual table
    --Name comes in as (table) tName.var so we can pass back out the name found PITA
    --returns the table found (key and value)
    local ld = {}
    local sMatch = ""
    local kMatch = nil
    local vMatch = nil

----FUNCTIONS for tableByName -----------------------------------------------------
    local function search4Str(n, k, v)
        local sKey = tostring(k)
        if string.find (n, sKey,1,true) then
            if sKey:len() > sMatch:len() then sMatch = sKey kMatch = k  vMatch = v end
            --find the longest match we can
        end
    end
----END FUNCTIONS for tableByName -------------------------------------------------

    if tName.val ~= nil and tName.val ~= "" then
        for k, v in pairs(_G) do
        --_G check both since some tables are only in _G or package.loaded
            search4Str(tName.val, k, v)
        end
        for k, v in pairs(package.loaded) do --package.loaded
            search4Str(tName.val, k, v)
        end
        if not string.find (sMatch, "_G",1,true) then sMatch = "_G." .. sMatch end
        -- put the root _G in if not exist
        if kMatch and vMatch then ld[kMatch] = vMatch tName.val = sMatch return ld end
    end
    tName.val = "_G"
    return package.loaded --Not Found return default
end

local function dump_Tables(tBase, sFunc, tSeen, tRet)
    --Based on: http://www.lua.org/cgi-bin/demo?globals
    --Recurse through tBase tables copying all found Tables
    local sSep=""
    local ld={}
    local tNameBuf = {}
    local sName
    if sFunc ~= "" then sSep = "." end

    for k, v in pairs(tBase) do
        k = tostring(k)
        tNameBuf[1] = sFunc
        tNameBuf[2] = sSep
        tNameBuf[3] = k


        if k ~= "loaded" and type(v) == "table" and not tSeen[v] then
            tSeen[v]=sFunc
            sName = table.concat(tNameBuf)
            tRet[sName] = a2m_m2a(v) --place all keys into ld[i]=value
            dump_Tables(v, sName, tSeen, tRet)
        elseif type(v) == "table" and not tSeen[v] then
            tSeen[v]=sFunc
            tRet[table.concat(tNameBuf)] = a2m_m2a(v) -- dump 'loaded' table
            for k1, v1 in pairs(v) do
                if not _G[k1] and type(v1) == "table" and not tSeen[v1] then
                    -- dump tables that are loaded but not global
                    tSeen[v1]=sFunc
                    tNameBuf[3] = k1
                    sName = table.concat(tNameBuf)
                    tRet[sName] = a2m_m2a(v1) --place all keys into ld[i]=value
                    dump_Tables(v1, sName, tSeen, tRet)
                end
            end
        end
    end
end

local function dump_Functions(tBase)
    --Based on: http://www.lua.org/cgi-bin/demo?globals
    --We already recursed through tBase copying all found tables
    --we look up the table by name and then (ab)use a2m_m2a() to load the address
    --after finding the table by address in tBase we will
        --put the table address of tFuncs in its place
    local tFuncBuf = {}
    for k,v in pairs(tBase) do
        local tTable = a2m_m2a(v)
        local tFuncs = {}

        for key, val in pairs(tTable) do
            if key ~= "loaded" then
                tFuncBuf[1] = dtTag(type(val))
                tFuncBuf[2] = tostring(key)
                tFuncs[table.concat(tFuncBuf)]= val
                --put the name and value in our tFuncs table
            end
        end
        tBase[k] = a2m_m2a(tFuncs) -- copy the address back to tBase
    end

end

local function get_common_branches(t, tRet)
    --load t 'names(values)' into keys
    --strip off long paths then iterate value if it exists
    --local tRet={}
    local sBranch = ""
    local tName = {}
    for k in pairs(t) do
            tName["val"]=k
            tableByName(tName)
            sBranch = tName.val
            if tRet[sBranch] == nil then
                tRet[sBranch] = 1 --first instance of this branch
            else
                tRet[sBranch] = tRet[sBranch] + 1
            end
    end
end

local function pairsByPairs (t, tkSorted)
    --tkSorted should be an already sorted (i)table with t[keys] in the values
    --https://www.lua.org/pil/19.3.html
    --!!Note: table sort default function does not like numbers as [KEY]!!
    --see *sortbyKeys*cmp_alphanum*

    local i = 0      -- iterator variable
    local iter = function ()   -- iterator function
        i = i + 1
        if tkSorted[i] == nil then return nil
            else return tkSorted[i], t[tkSorted[i]]
        end
    end
    return iter
end

local function sortbyKeys(t, tkSorted)
    --loads keys of (t) into values of tkSorted
    --and then sorts them
    --tkSorted has integer keys (see ipairs)
----FUNCTIONS for sortByKeys -------------
    local cmp_alphanum = function (op1, op2)
                            local type1= type(op1)
                            local type2 = type(op2)
                            if type1 ~= type2 then
                                return type1 < type2
                            else
                                return op1 < op2
                            end
                        end
----END FUNCTIONS for sortByKeys ---------
    for n in pairs(t) do table.insert(tkSorted, n) end
    table.sort(tkSorted, cmp_alphanum)--table.sort(tkSorted)
end

local function funcprint(tBuf, strName, value)
        local sType = type(value)
        local sVal = ""
        local sHex = ""
    tBuf[#tBuf + 1] = "\t"
    tBuf[#tBuf + 1] = strName
    if nil ~= string.find (";string;number;userdata;boolean;", sType, 1, true) then
        --If any of the above types print the contents of variable
        sVal = tostring(value)

        if type(value) == "number" then
            sHex = " = 0x" .. string.format("%x", value)
        else
            sHex = ""
            sVal = string.gsub(sVal, "\n", "\\n") --replace newline with \n
        end
        tBuf[#tBuf + 1] = " : "
        tBuf[#tBuf + 1] = sVal
        tBuf[#tBuf + 1] = sHex
    end
    tBuf[#tBuf + 1] = "\r\n"
end

local function errorHandler( err )
    filehandle:write(" ERROR:" .. err .. "\n")
end


------------MAIN----------------------------------------------------------------
    local _NIL = nil
    local tSeen= {}
    local tcBase = {}
    local tkSortCbase = {}
    local tMods= {}
    local tkSortMods = {}
    local tWriteBuf = {}
    local n = 0 -- count of how many items were found

    filehandle = io.open(sDumpFile, "w+") --overwrite
    tWriteBuf[#tWriteBuf + 1] = "*Loaded Modules* \n"

    xpcall( function()
                dump_Tables(tableByName({["val"] = "_G"}),"", tSeen, tMods)
                --you can put a table name here if you just wanted to display
                --only its items, ex. "os" or "rb" or "io"
                --However, it has to be accessible directly from _G
                --so "rb.actions" wouldn't return anything since its technically
                --enumerated through _G.rb
            end , errorHandler )
    tSeen = nil

    xpcall( function()dump_Functions(tMods)end , errorHandler )

    get_common_branches(tMods, tcBase)

    sortbyKeys(tcBase, tkSortCbase)
    sortbyKeys(tMods, tkSortMods)

    for k, v in pairsByPairs(tcBase, tkSortCbase ) do
        n = n + 1
        if n ~= 1 then
            tWriteBuf[#tWriteBuf + 1] = ", "
        end
        tWriteBuf[#tWriteBuf + 1] = tostring(k)
        if n >= 3 then -- split loaded modules to multiple lines
            n = 0
            tWriteBuf[#tWriteBuf + 1] = "\r\n"
        end
        if #tWriteBuf > 25 then
            filehandle:write(table.concat(tWriteBuf))
            for i=1, #tWriteBuf do tWriteBuf[i] = _NIL end -- reuse table
        end
    end
    if ... then
        tcBase= nil tkSortCbase= nil
    end
    tWriteBuf[#tWriteBuf + 1] = "\r\n"
    tWriteBuf[#tWriteBuf + 1] = dtTag("?")
    tWriteBuf[#tWriteBuf + 1] = "\r\n\r\n"
    tWriteBuf[#tWriteBuf + 1] = "Functions: \r\n"

    n = 0
    for key, val in pairsByPairs(tMods, tkSortMods) do
        local tkSorted = {}
        local tFuncs = a2m_m2a(val)
        sortbyKeys(tFuncs, tkSorted)
        tWriteBuf[#tWriteBuf + 1] = "\r\n"
        tWriteBuf[#tWriteBuf + 1] = tostring(key)
        tWriteBuf[#tWriteBuf + 1] = "\r\n"
        for k, v in pairsByPairs(tFuncs, tkSorted) do
            n = n + 1
            funcprint(tWriteBuf, k,v)
            if #tWriteBuf > 25 then
                filehandle:write(table.concat(tWriteBuf))
                for i=1, #tWriteBuf do tWriteBuf[i] = _NIL end -- reuse table
            end
        end
    end
    tWriteBuf[#tWriteBuf + 1] = "\r\n\r\n"
    tWriteBuf[#tWriteBuf + 1] = n
    tWriteBuf[#tWriteBuf + 1] = " Items Found \r\n"
    filehandle:write(table.concat(tWriteBuf))
    for i=1, #tWriteBuf do tWriteBuf[i] = _NIL end -- empty table
    filehandle:close()
    --rb.splash((rb.HZ or 100) * 5, n .. " Items dumped to : " .. sDumpFile)
    --rb.splash(500, collectgarbage("count"))
if not ... then
    local lu = collectgarbage("collect")
    local used, allocd, free = rb.mem_stats()
    local lu = collectgarbage("count")
    local fmt = function(t, v) return string.format("%s: %d Kb\n", t, v /1024) end
    local s_t = {}
    s_t[1] = n
    s_t[2] = " Items dumped to:\n"
    s_t[3] = sDumpFile
    s_t[4] = "\n\nLoaded Modules:\n"
    n = 0
    for k, v in pairsByPairs(tcBase, tkSortCbase ) do
        n = n + 1
        if n ~= 1 then
            s_t[#s_t + 1] = ", "
        end
        s_t[#s_t + 1] = tostring(k)
        if n >= 3 then -- split loaded modules to multiple lines
            n = 0
            s_t[#s_t + 1] = "\n"
        end
    end
    rb.splash_scroller(5 * (rb.HZ or 100), table.concat(s_t))
end
