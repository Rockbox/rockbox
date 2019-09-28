rb = rb or {}
rb.create_numbered_filename = function (sPath, sPrefix, sSuffix, iNumLen, iNum)
    iNum = iNum or -1
    local dir_iter, dir_data = luadir.dir(sPath)
    local status = true
    local name, isdir, num
    local name_pat = sPrefix .. '(%d+)' .. sSuffix
    local file_pat
    local max_num = iNum < 0 and -1 or iNum -- Number specified

    if max_num < 0 then
        max_num = 0 -- automatic numbering
        repeat
            status, name, isdir = pcall(dir_iter, dir_data)
            if status then
                if name and not isdir then
                    num = string.match(name, name_pat)
                    if (not iNumLen) and num then -- try to match existing zero padding
                        local s, e = string.find(num, "^0+")
                        if s and e then iNumLen = (e - s) end
                    end
                    num = tonumber(num)
                    if num and (num > max_num) then 
                        max_num = num
                    end
                end
            end
        until not status
    end
    max_num = max_num + 1
    iNumLen = iNumLen or 0
    file_pat = "%s/%s%0" .. iNumLen .. "d%s"
    return string.format(file_pat, sPath, sPrefix, max_num, sSuffix), max_num
end

rb.strip_extension = function (sFileName)
    sFileName = sFileName or ""
    local ext = rb.strrchr(sFileName, string.byte("."));
    local len = string.len(ext or "")
    if len > 0 then sFileName = string.sub(sFileName, 1, -(len + 1)) end
    return sFileName
end
