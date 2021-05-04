--[[
temp loader allows some lua requires to be loaded and later garbage collected
unfortunately the module needs to be formatted in such a way to pass back a 
call table in order to keep the functions within from being garbage collected 
too early

BE AWARE this bypasses the module loader which would allow code reuse
so if you aren't careful this memory saving tool could spell disaster
for free RAM if you load the same code multiple times
--]]


local function tempload(modulename)
  --http://lua-users.org/wiki/LuaModulesLoader
  local errmsg = ""
  -- Find source
  local modulepath = string.gsub(modulename, "%.", "/")
  for path in string.gmatch(package.path, "([^;]+)") do
    local filename = string.gsub(path, "%?", modulepath)
    local file = io.open(filename, "r")
    if file then
      -- Compile and return the module
      return assert(loadstring(assert(file:read("*a")), filename))()
    end
    errmsg = errmsg.."\n\tno file '"..filename.."' (temp loader)"
  end
  return errmsg
end

return tempload
