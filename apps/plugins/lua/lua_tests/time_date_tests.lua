--[[
             __________               __   ___.
   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
                     \/            \/     \/    \/            \/
 $Id$

 Copyright (C) 2026 William Wilgus
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 KIND, either express or implied.
]]--

if not os.time() then rb.splash(rb.HZ, "No Support!") return nil end

local timefns = require("strftime")
-- Random timestamps with expected UTC tm fields
-- yday is 0-based, wday is 1=Sunday

local tests = {
{ts = 0, --Thu Jan  1 00:00:00 1970
 tm = {year=70, month=0, day=1, hour=0, min=0, sec=0, wday=4, yday=0}},
{ts = 15620300, --Tue Jun 30 18:58:20 1970
 tm = {year=70, month=5, day=30, hour=18, min=58, sec=20, wday=2, yday=180}},
{ts = 70679700, --Wed Mar 29 01:15:00 1972
 tm = {year=72, month=2, day=29, hour=1, min=15, sec=0, wday=3, yday=88}},
{ts = 118231000, --Sun Sep 30 09:56:40 1973
 tm = {year=73, month=8, day=30, hour=9, min=56, sec=40, wday=0, yday=272}},
{ts = 135663600, --Sat Apr 20 04:20:00 1974
 tm = {year=74, month=3, day=20, hour=4, min=20, sec=0, wday=6, yday=109}},
{ts = 142740200, --Thu Jul 11 02:03:20 1974
 tm = {year=74, month=6, day=11, hour=2, min=3, sec=20, wday=4, yday=191}},
{ts = 161467300, --Wed Feb 12 20:01:40 1975
 tm = {year=75, month=1, day=12, hour=20, min=1, sec=40, wday=3, yday=42}},
{ts = 163538500, --Sat Mar  8 19:21:40 1975
 tm = {year=75, month=2, day=8, hour=19, min=21, sec=40, wday=6, yday=66}},
{ts = 199525600, --Wed Apr 28 07:46:40 1976
 tm = {year=76, month=3, day=28, hour=7, min=46, sec=40, wday=3, yday=118}},
{ts = 251219300, --Sat Dec 17 15:08:20 1977
 tm = {year=77, month=11, day=17, hour=15, min=8, sec=20, wday=6, yday=350}},
{ts = 265027300, --Fri May 26 10:41:40 1978
 tm = {year=78, month=4, day=26, hour=10, min=41, sec=40, wday=5, yday=145}},
{ts = 274951800, --Mon Sep 18 07:30:00 1978
 tm = {year=78, month=8, day=18, hour=7, min=30, sec=0, wday=1, yday=260}},
{ts = 286861200, --Sat Feb  3 03:40:00 1979
 tm = {year=79, month=1, day=3, hour=3, min=40, sec=0, wday=6, yday=33}},
{ts = 299892500, --Tue Jul  3 23:28:20 1979
 tm = {year=79, month=6, day=3, hour=23, min=28, sec=20, wday=2, yday=183}},
{ts = 324488000, --Sun Apr 13 15:33:20 1980
 tm = {year=80, month=3, day=13, hour=15, min=33, sec=20, wday=0, yday=103}},
{ts = 369450300, --Wed Sep 16 01:05:00 1981
 tm = {year=81, month=8, day=16, hour=1, min=5, sec=0, wday=3, yday=258}},
{ts = 423042600, --Sun May 29 07:50:00 1983
 tm = {year=83, month=4, day=29, hour=7, min=50, sec=0, wday=0, yday=148}},
{ts = 442891600, --Sat Jan 14 01:26:40 1984
 tm = {year=84, month=0, day=14, hour=1, min=26, sec=40, wday=6, yday=13}},
{ts = 449968200, --Wed Apr  4 23:10:00 1984
 tm = {year=84, month=3, day=4, hour=23, min=10, sec=0, wday=3, yday=94}},
{ts = 468609000, --Tue Nov  6 17:10:00 1984
 tm = {year=84, month=10, day=6, hour=17, min=10, sec=0, wday=2, yday=310}},
{ts = 477670500, --Tue Feb 19 14:15:00 1985
 tm = {year=85, month=1, day=19, hour=14, min=15, sec=0, wday=2, yday=49}},
{ts = 481295100, --Tue Apr  2 13:05:00 1985
 tm = {year=85, month=3, day=2, hour=13, min=5, sec=0, wday=2, yday=91}},
{ts = 543517400, --Mon Mar 23 17:03:20 1987
 tm = {year=87, month=2, day=23, hour=17, min=3, sec=20, wday=1, yday=81}},
{ts = 599439800, --Thu Dec 29 23:03:20 1988
 tm = {year=88, month=11, day=29, hour=23, min=3, sec=20, wday=4, yday=363}},
{ts = 653808800, --Thu Sep 20 05:33:20 1990
 tm = {year=90, month=8, day=20, hour=5, min=33, sec=20, wday=4, yday=262}},
{ts = 670723600, --Thu Apr  4 00:06:40 1991
 tm = {year=91, month=3, day=4, hour=0, min=6, sec=40, wday=4, yday=93}},
{ts = 718274900, --Mon Oct  5 08:48:20 1992
 tm = {year=92, month=9, day=5, hour=8, min=48, sec=20, wday=1, yday=278}},
{ts = 725351500, --Sat Dec 26 06:31:40 1992
 tm = {year=92, month=11, day=26, hour=6, min=31, sec=40, wday=6, yday=360}},
{ts = 760389300, --Fri Feb  4 19:15:00 1994
 tm = {year=94, month=1, day=4, hour=19, min=15, sec=0, wday=5, yday=34}},
{ts = 763841300, --Wed Mar 16 18:08:20 1994
 tm = {year=94, month=2, day=16, hour=18, min=8, sec=20, wday=3, yday=74}},
{ts = 765135800, --Thu Mar 31 17:43:20 1994
 tm = {year=94, month=2, day=31, hour=17, min=43, sec=20, wday=4, yday=89}},
{ts = 814930900, --Sun Oct 29 01:41:40 1995
 tm = {year=95, month=9, day=29, hour=1, min=41, sec=40, wday=0, yday=301}},
{ts = 834003200, --Wed Jun  5 19:33:20 1996
 tm = {year=96, month=5, day=5, hour=19, min=33, sec=20, wday=3, yday=156}},
{ts = 890270800, --Thu Mar 19 01:26:40 1998
 tm = {year=98, month=2, day=19, hour=1, min=26, sec=40, wday=4, yday=77}},
{ts = 924445600, --Sun Apr 18 14:26:40 1999
 tm = {year=99, month=3, day=18, hour=14, min=26, sec=40, wday=0, yday=107}},
{ts = 960864200, --Tue Jun 13 02:43:20 2000
 tm = {year=100, month=5, day=13, hour=2, min=43, sec=20, wday=2, yday=164}},
{ts = 961123100, --Fri Jun 16 02:38:20 2000
 tm = {year=100, month=5, day=16, hour=2, min=38, sec=20, wday=5, yday=167}},
{ts = 1014025000, --Mon Feb 18 09:36:40 2002
 tm = {year=102, month=1, day=18, hour=9, min=36, sec=40, wday=1, yday=48}},
{ts = 1052514800, --Fri May  9 21:13:20 2003
 tm = {year=103, month=4, day=9, hour=21, min=13, sec=20, wday=5, yday=128}},
{ts = 1088760800, --Fri Jul  2 09:33:20 2004
 tm = {year=104, month=6, day=2, hour=9, min=33, sec=20, wday=5, yday=183}},
{ts = 1130184800, --Mon Oct 24 20:13:20 2005
 tm = {year=105, month=9, day=24, hour=20, min=13, sec=20, wday=1, yday=296}},
{ts = 1182396300, --Thu Jun 21 03:25:00 2007
 tm = {year=107, month=5, day=21, hour=3, min=25, sec=0, wday=4, yday=171}},
{ts = 1228566800, --Sat Dec  6 12:33:20 2008
 tm = {year=108, month=11, day=6, hour=12, min=33, sec=20, wday=6, yday=340}},
{ts = 1281900200, --Sun Aug 15 19:23:20 2010
 tm = {year=110, month=7, day=15, hour=19, min=23, sec=20, wday=0, yday=226}},
{ts = 1284057700, --Thu Sep  9 18:41:40 2010
 tm = {year=110, month=8, day=9, hour=18, min=41, sec=40, wday=4, yday=251}},
{ts = 1291652100, --Mon Dec  6 16:15:00 2010
 tm = {year=110, month=11, day=6, hour=16, min=15, sec=0, wday=1, yday=339}},
{ts = 1326776200, --Tue Jan 17 04:56:40 2012
 tm = {year=112, month=0, day=17, hour=4, min=56, sec=40, wday=2, yday=16}},
{ts = 1382439700, --Tue Oct 22 11:01:40 2013
 tm = {year=113, month=9, day=22, hour=11, min=1, sec=40, wday=2, yday=294}},
{ts = 1409796800, --Thu Sep  4 02:13:20 2014
 tm = {year=114, month=8, day=4, hour=2, min=13, sec=20, wday=4, yday=246}},
{ts = 1451997500, --Tue Jan  5 12:38:20 2016
 tm = {year=116, month=0, day=5, hour=12, min=38, sec=20, wday=2, yday=4}},
{ts = 1463216500, --Sat May 14 09:01:40 2016
 tm = {year=116, month=4, day=14, hour=9, min=1, sec=40, wday=6, yday=134}},
{ts = 1499635100, --Sun Jul  9 21:18:20 2017
 tm = {year=117, month=6, day=9, hour=21, min=18, sec=20, wday=0, yday=189}},
{ts = 1545460400, --Sat Dec 22 06:33:20 2018
 tm = {year=118, month=11, day=22, hour=6, min=33, sec=20, wday=6, yday=355}},
{ts = 1555816400, --Sun Apr 21 03:13:20 2019
 tm = {year=119, month=3, day=21, hour=3, min=13, sec=20, wday=0, yday=110}},
{ts = 1585072100, --Tue Mar 24 17:48:20 2020
 tm ={year=120, month=2, day=24, hour=17, min=48, sec=20, wday=2, yday=83}},
{ts = 1622181100, --Fri May 28 05:51:40 2021
 tm = {year=121, month=4, day=28, hour=5, min=51, sec=40, wday=5, yday=147}},
{ts = 1649451900, --Fri Apr  8 21:05:00 2022
 tm = {year=122, month=3, day=8, hour=21, min=5, sec=0, wday=5, yday=97}},
{ts = 1663173600, --Wed Sep 14 16:40:00 2022
 tm = {year=122, month=8, day=14, hour=16, min=40, sec=0, wday=3, yday=256}},
{ts = 1707272900, --Wed Feb  7 02:28:20 2024
 tm = {year=124, month=1, day=7, hour=2, min=28, sec=20, wday=3, yday=37}},
{ts = 1769495200, --Tue Jan 27 06:26:40 2026
 tm = {year=126, month=0, day=27, hour=6, min=26, sec=40, wday=2, yday=26}},
{ts = 1786582600, --Thu Aug 13 00:56:40 2026
 tm = {year=126, month=7, day=13, hour=0, min=56, sec=40, wday=4, yday=224}},
{ts = 1831890100, --Wed Jan 19 10:21:40 2028
 tm = {year=128, month=0, day=19, hour=10, min=21, sec=40, wday=3, yday=18}},
{ts = 1880735900, --Mon Aug  6 18:38:20 2029
 tm = {year=129, month=7, day=6, hour=18, min=38, sec=20, wday=1, yday=217}},
{ts = 1916809300, --Sat Sep 28 07:01:40 2030
 tm = {year=130, month=8, day=28, hour=7, min=1, sec=40, wday=6, yday=270}},
{ts = 1955299100, --Wed Dec 17 18:38:20 2031
 tm = {year=131, month=11, day=17, hour=18, min=38, sec=20, wday=3, yday=350}},
{ts = 1975234400, --Wed Aug  4 12:13:20 2032
 tm = {year=132, month=7, day=4, hour=12, min=13, sec=20, wday=3, yday=216}},
{ts = 1984554800, --Sat Nov 20 09:13:20 2032
 tm = {year=132, month=10, day=20, hour=9, min=13, sec=20, wday=6, yday=324}},
{ts = 2023217200, --Fri Feb 10 20:46:40 2034
 tm = {year=134, month=1, day=10, hour=20, min=46, sec=40, wday=5, yday=40}},
{ts = 2032882800, --Fri Jun  2 17:40:00 2034
 tm = {year=134, month=5, day=2, hour=17, min=40, sec=0, wday=5, yday=152}},
{ts = 2080606700, --Fri Dec  7 02:18:20 2035
 tm = {year=135, month=11, day=7, hour=2, min=18, sec=20, wday=5, yday=340}},
{ts = 2092429800, --Mon Apr 21 22:30:00 2036
 tm = {year=136, month=3, day=21, hour=22, min=30, sec=0, wday=1, yday=111}},
{ts = 2131903647, --Wednesday July 22, 2037 19:27:27 (pm) in time zone UTC (UTC)
 tm = {year=137, month=6, day=22, hour=19, min=27, sec=27, wday=3, yday=202}},

}

local function check_field(entry, timestamp, name, got, exp)
    if not got or not exp then
        return false , string.format("entry: %d ts: %d error: key[%s] does not exist", entry, timestamp, name)
    end
    if got ~= exp then
        return false, string.format("entry: %d ts: %d error: key[%s] mismatch got %d, expected %d",
                                    entry, timestamp, name, got, exp)
    end
    return true
end
local passed = 0;
local ok = true;

for _, test in ipairs(tests) do
    local s_t = {}
    s_t[1] = ""
    local out = {}
    timefns.gmtime(test.ts, out)

    for k, exp in pairs(test.tm) do

        local pass, err = check_field(passed, test.ts, k, out[k], exp)
        if not pass then
            ok = false
            s_t[#s_t + 1] = err;
            break
        end
    end

    if not ok then
        s_t[1] = "FAIL"
        rb.splash_scroller(10 * rb.HZ, table.concat(s_t, "\n"))
        break;
    end
    passed = passed + 1
end

if ok then
    rb.splash(rb.HZ * 5, tostring(passed) .. " / " .. #tests .. " timestamps match")
end
