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
 
// This is the array of targets, with the target id as the key and the 
// plaintext name of the target as the value
$targets = array( 'archosfmrecorder' => 'Archos FM Recorder',
                  'archosondiofm' => 'Archos Ondio FM',
                  'archosondiosp' => 'Archos Ondio SP',
                  'archosplayer' => 'Archos Player/Studio',
                  'archosrecorder' => 'Archos Recorder v1',
                  'archosrecorderv2' => 'Archos Recorder v2',
                  'cowond2' => 'Cowon D2',
                  'iaudiom3' => 'iAudio M3',
                  'iaudiom5' => 'iAudio M5',
                  'iaudiox5' => 'iAudio X5',
                  'ipod1g2g' => 'iPod 1st and 2nd gen',
                  'ipod3g' => 'iPod 3rd gen',
                  'ipod4g' => 'iPod 4th gen Grayscale',
                  'ipodcolor' => 'iPod Color/Photo',
                  'ipodmini1g' => 'iPod Mini 1st gen',
                  'ipodmini2g' => 'iPod Mini 2nd gen',
                  'ipodnano1g' => 'iPod Nano 1st gen',
                  'ipodnano2g' => 'iPod Nano 2nd gen',
                  'ipodvideo' => 'iPod Video',
                  'iriverh10' => 'iriver H10 20GB',
                  'iriverh10_5gb' => 'iriver H10 5GB',
                  'iriverh100' => 'iriver H100/115',
                  'iriverh120' => 'iriver H120/140',
                  'iriverh300' => 'iriver H320/340',
                  'mrobe100' => 'Olympus M-Robe 100',
                  'mrobe500' => 'Olympus M-Robe 500',
                  'vibe500' => 'Packard Bell Vibe 500',
                  'samsungyh820' => 'Samsung YH-820',
                  'samsungyh920' => 'Samsung YH-920',
                  'samsungyh925' => 'Samsung YH-925',
                  'sansac200' => 'SanDisk Sansa c200',
                  'sansaclip' => 'SanDisk Sansa Clip v1',
                  'sansaclipv2' => 'SanDisk Sansa Clip v2',
                  'sansaclipplus' => 'SanDisk Sansa Clip+',
                  'sansae200' => 'SanDisk Sansa e200',
                  'sansae200v2' => 'SanDisk Sansa e200 v2',
                  'sansafuze' => 'SanDisk Sansa Fuze',
                  'sansafuzev2' => 'SanDisk Sansa Fuze v2',
                  'gigabeatfx' => 'Toshiba Gigabeat F/X',
                  'gigabeats' => 'Toshiba Gigabeat S'
                );

// Looping through all the targets
foreach($targets as $target => $plaintext)
{
    // Opening a cpp process
    $configfile = '../../firmware/export/config/' . $target . '.h';
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
    
    // Header for the target
    echo $target . "\n{\n";
    echo '    name   : ' . $plaintext . "\n";
    
    // Writing the LCD dimensions
    echo '    screen : ' . $results[7] . ' x ' . $results[8] . ' @ ';
    if($results[9] == 1)
        echo 'mono';
    else if($results[10] == 2)
        echo 'grey';
    else
        echo 'rgb';
    echo "\n";
    
    // Writing the remote dimensions if necessary
    echo '    remote : ';
    if($results[12] == 0)
    {
        echo 'no';
    }
    else
    {
        echo $results[12] . ' x ' .$results[13] . ' @ ';
        if($results[14] == 1)
            echo 'mono';
        else if($results[14] == 2)
            echo 'grey';
        else
            echo 'rgb';
    }
    echo "\n";
    
    // Writing FM capability
    echo '    fm     : ';
    if($results[18] == 'yes')
        echo 'yes';
    else
        echo 'no';
    echo "\n";
    
    // Writing record capability
    echo '    record : ';
    if($results[22] == 'yes')
        echo 'yes';
    else
        echo 'no';
    echo "\n";
    
    // Closing the target
    echo "}\n\n";
    
    proc_close($proc);
}

?> 
