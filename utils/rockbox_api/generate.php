#!/usr/bin/php
<?
require_once("functions.php");

echo   '# Auto generated documentation by Rockbox plugin API generator v2'."\n";
echo   '# Made by Maurus Cuelenaere'."\n";
echo <<<MOO
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# \$Id$
#
# Generated from $svn\x61pps/plugin.h
#
# Format:
# \\group memory and strings
# \\conditions defined(HAVE_BACKLIGHT)
# \\param fmt
# \\return
# \\description
# \\see func1 func2 [S[apps/plugin.c]]
#
# Markup:
# [W[wiki url]]
# [S[svn url]]
# [F[function]]
# [[url]]
# %BR%
# =code=

MOO;

foreach(get_newest() as $line)
{
    echo "\n".clean_func($line["func"])."\n";

    if(strlen($line["group"]) > 0)
        echo "    \\group ".$line["group"]."\n";

    if(strlen($line["cond"]) > 2)
        echo "    \\conditions "._simplify($line["cond"])."\n";

    foreach(get_args($line["func"]) as $param)
    {
        if(strlen($param) > 0 && $param != "...")
        {
            $param = split_var($param);
            $param = $param[1];
            echo "    \\param $param\n";
        }
    }

    if(get_return($line["func"]) !== false)
        echo "    \\return\n";

    echo "    \\description\n";
}

echo "\n# END\n";
?>