#!/usr/bin/php -q
<?php
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Robert Bieber
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
 
if($argc < 2)
    $path = getcwd();
else
    $path = $argv[1];
$path = rtrim($path, "/");
$dir = dir($path);
$split = explode("/", $path);
$last = $split[count($split) - 1];
echo "\t<qresource prefix=\"/$last\">\n";
while(false !== ($entry = $dir->read()))
{
    if($entry == '.' || $entry == '..')
        continue;
    echo "\t\t";
    echo "<file alias = \"$entry\">";
    echo $path . '/' . $entry . "</file>";
    echo "\n";
}
echo "\t</qresource>\n";
$dir->close();

?>
