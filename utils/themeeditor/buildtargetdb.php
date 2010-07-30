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
 
// Printing the do-not-modify warning
echo "# ----------------------------------------------------------- #\n";
echo "# ----------------------------------------------------------- #\n";
echo "# --- This file automatically generated, do not modify!   --- #\n";
echo "# ----------------------------------------------------------- #\n";
echo "# ----------------------------------------------------------- #\n\n";
 
// Importing the target array
system('./includetargets.pl');
require('./targets.php');
unlink('./targets.php');

// Looping through all the targets
foreach($targets as $target => $plaintext)
{
    // Opening a cpp process
    $configfile = '../../firmware/export/config/' . $target . '.h';
    if(!file_exists($configfile))
        continue;
    $descriptor = array( 0 => array("pipe", "r"), //stdin
                         1 => array("pipe", "w") //stdout
                        );
                        
    $proc = proc_open('cpp', $descriptor, $pipes);
    
    if($proc == false)
        die("Failed to open process");

    // Feeding the input to cpp
    $input = "#include \"$configfile\"\n";
    $input .= <<<STR
lcd
LCD_WIDTH
LCD_HEIGHT
LCD_DEPTH
remote
#ifdef HAVE_REMOTE_LCD
LCD_REMOTE_WIDTH
LCD_REMOTE_HEIGHT
LCD_REMOTE_DEPTH
#endif
tuner
#ifdef CONFIG_TUNER
yes
#endif
recording
#ifdef HAVE_RECORDING
yes
#endif
unused
STR;

    fwrite($pipes[0], $input);
    fclose($pipes[0]);
    
    $results = stream_get_contents($pipes[1]);
    fclose($pipes[1]);
    $results = explode("\n", $results);
    
    $base = 0;
    while($results[$base] != 'lcd')
        $base++;
    
    // Header for the target
    echo $target . "\n{\n";
    echo '    name   : ' . $plaintext . "\n";
    
    // Writing the LCD dimensions
    echo '    screen : ' . $results[$base + 1] . ' x ' . $results[$base + 2] . ' @ ';
    if($results[$base + 3] == 1)
        echo 'mono';
    else if($results[$base + 3] == 2)
        echo 'grey';
    else
        echo 'rgb';
    echo "\n";
    
    // Writing the remote dimensions if necessary
    echo '    remote : ';
    if($results[$base + 6] == 0)
    {
        echo 'no';
    }
    else
    {
        echo $results[$base + 6] . ' x ' .$results[$base + 7] . ' @ ';
        if($results[$base + 8] == 1)
            echo 'mono';
        else if($results[$base + 8] == 2)
            echo 'grey';
        else
            echo 'rgb';
    }
    echo "\n";
    
    // Writing FM capability
    echo '    fm     : ';
    if($results[$base + 12] == 'yes')
        echo 'yes';
    else
        echo 'no';
    echo "\n";
    
    // Writing record capability
    echo '    record : ';
    if($results[$base + 16] == 'yes')
        echo 'yes';
    else
        echo 'no';
    echo "\n";
    
    // Closing the target
    echo "}\n\n";
    
    proc_close($proc);
}

?> 
