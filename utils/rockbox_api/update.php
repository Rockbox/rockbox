#!/usr/bin/php
<?
require_once("functions.php");

$input = file_get_contents($argv[1]);

$input = parse_documentation($input);

/* Format input */
foreach($input as $rootname => $rootel)
{
    foreach($rootel as $name => $el)
        $input[$name] = $el;
    unset($input[$rootname]);
}

$new = get_newest();

/* Format new */
foreach($new as $name => $el)
{
    unset($new[$name]);
    $name = clean_func($el["func"]);
    
    $new[$name] = array(
                    "group" => array($el["group"]),
                    "description" => array("")
    );
    
    if(strlen($el["cond"]) > 2)
        $new[$name]["conditions"][0] = $el["cond"];
    
    $args = get_args($el["func"]);
    if(count($args) > 0)
    {
        foreach($args as $n => $arg)
        {
            $tmp = split_var($arg);
            $args[$n] = $tmp[1];
        }
        $new[$name]["param"] = $args;
    }
    
    if(get_return($el["func"]) !== false)
        $new[$name]["return"][0] = "";
}

/* Compare and merge both */
$merged = array();
foreach($new as $name => $el)
{
    if(isset($input[$name]))
    {
        $merged[$name] = $input[$name];
        $merged[$name]["conditions"] = $new[$name]["conditions"];
        
        if(strlen($el["group"][0]) > 0)
            $merged[$name]["group"] = $el["group"];
        
        if(isset($el["param"]))
        {
            foreach($el["param"] as $nr => $parel)
            {
                if($parel != $input[$name]["param"][$nr])
                {
                    $param = trim($parel);
                    $p1 = substr($param, 0, strpos($param, " "));
                    
                    $param = trim($input[$name]["param"][$nr]);
                    $p2 = substr($param, strpos($param, " "));
                    $merged[$name]["params"][] = $p1." ".$p2." [AUTO-ADDED]";
                }
                else
                    $merged[$name]["params"][] = $parel;
            }
        }
        
        if(!isset($el["return"]) && isset($merged[$name]["return"]))
            unset($merged[$name]["return"]);
        
        unset($input[$name]);
    }
    else
        $merged[$name] = $el;
}

/* Now to the rest of input */
foreach($input as $name => $el)
    $merged[$name." [DEPRECATED]"] = $el;

uksort($merged, "func_sort");

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

foreach($merged as $func => $line)
{
    echo "\n".clean_func($func)."\n";

    if(strlen($line["group"]) > 0)
        echo "    \\group ".trim($line["group"][0])."\n";

    if(strlen($line["conditions"]) > 2)
        echo "    \\conditions ".trim(_simplify($line["conditions"][0]))."\n";

    if(isset($line["param"]))
    {
        foreach($line["param"] as $param)
        {
            if($param != "...")
                echo "    \\param ".trim($param)."\n";
        }
    }

    if(isset($line["return"]))
    {
        if(trim($line["return"]) == "")
            echo "    \\return\n";
        else
            echo "    \\return ".trim($line["return"][0])."\n";
    }
    
    if(trim($line["description"]) == "")
        echo "    \\description\n";
    else
        echo "    \\description ".trim($line["description"][0])."\n";
    
    if(isset($line["see"]))
        echo "    \\see ".trim($line["see"][0])."\n";
}

echo "\n# END\n";
?>