/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Franklin Wei, Benjamin Brown
 * Copyright (C) 2004 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#include "plugin.h"
#include "engine.h"
#include "sys.h"
#include "util.h"

/* we don't want these on the stack, they're big and could cause a stack overflow */
static struct Engine e;
static struct System sys;

enum plugin_status plugin_start(const void* parameter)
{
    (void) parameter;

    /* no trailing slashes */
    const char *dataPath = ROCKBOX_DIR "/xworld";
    const char *savePath = ROCKBOX_DIR "/xworld";
    g_debugMask = XWORLD_DEBUGMASK;

    engine_create(&e, &sys, dataPath, savePath);
    engine_init(&e);
    sys_menu(&sys);

    engine_run(&e);

    engine_finish(&e);
    return PLUGIN_OK;
}



/*
   Game was originally made with 16. SIXTEEN colors. Running on 320x200 (64,000 pixels.)

   Great fan site here: https://sites.google.com/site/interlinkknight/anotherworld
   Contains the wheelcode :P !

   A lot of details can be found regarding the game and engine architecture at:
   http://www.anotherworld.fr/anotherworld_uk/another_world.htm

   The chronology of the game implementation can retraced via the ordering of the opcodes:
   The sound and music opcode are at the end: Music and sound was done at the end.

   Call tree:
   =========

   SDLSystem       systemImplementaion ;
   System *sys = & systemImplementaion ;

   main
   {

       Engine *e = new Engine();
           e->run()
           {
              sys->init("Out Of This World");
              setup();
              vm.restartAt(0x3E80); // demo starts at 0x3E81

             while (!_stub->_pi.quit)
                 {
                   vm.setupScripts();
                   vm.inp_updatePlayer();
                    processInput();
                   vm.runScripts();
             }

             finish();
           }
   }


   Virtual Machine:
   ================

           Seems the threading model is collaborative multi-tasking (as opposed to preemptive multitasking):
           A thread (called a Channel on Eric Chahi's website) will release the hand to the next one via the
           break opcode.

           It seems that even when a setvec is requested by a thread, we cannot set the instruction pointer
           yet. The thread is allowed to keep on executing its code for the remaining of the vm frame.

           A virtual machine frame has a variable duration. The unit of time is 20ms and the frame can be set
           to live for 1 (20ms ; 50Hz) up to 5 (100ms ; 10Hz).


           There are 30 something opcodes. The graphic opcode are more complex, not only the declare the operation to perform
           they also define where to find the vertices (segVideo1 or segVideo2).

           No stack available but a thread can save its pc (Program Counter) once: One method call and return is possible.


   Video :
   =======
           Double buffer architecture. AW opcodes even has a special instruction for blitting from one
           frame buffer to an other.

           Double buffering is implemented in software

           According to Eric Chahi's webpage there are 4 framebuffer. Since on full screenbuffer is 320x200/2 = 32kB
           that would mean the total size consumed is 128KB ?

   Sound :
   =======
           Mixing is done on software.

           Since the virtual machine and SDL are running simultaneously in two different threads:
           Any read or write to an elements of the sound channels MUST be synchronized with a
           mutex.

   FastMode :
   ==========

   The game engine features a "fast-mode"...what it to be able to respond to the now defunct
   TURBO button commonly found on 386/486 era PC ?!


   Endianess:
   ==========

   Atari and Amiga used bigEndian CPUs. Data are hence stored within BANK in big endian format.
   On an Intel or ARM CPU data will have to be transformed when read.



   The original codebase contained a looooot of cryptic hexa values.
   0x100 (for 256 variables)
   0x400 (for one kilobyte)
   0x40 (for num threads)
   0x3F (num thread mask)
   I cleaned that up.

   Questions & Answers :
   =====================

   Q: How does the interpreter deals with the CPU speed ?! A pentium is a tad faster than a Motorola 68000
      after all.
   A: See vm frame time: The vm frame duration is variable. The vm actually write for how long a video frame
      should be displayed in variable 0xFF. The value is the number of 20ms slice

   Q: Why is a palette 2048 bytes if there are only 16 colors ? I would have expected 48 bytes...
   A: ???

   Q: Why does Resource::load() search for ressource to load from higher to lower....since it will load stuff
      until no more ressources are marked as "Need to be loaded".
   A: ???

   Original DOS version :
   ======================

   Banks: 1,236,519 B
   exe  :    20,293 B


   Total bank      size: 1236519 (100%)
   ---------------------------------
   Total RT_SOUND    size: 585052  ( 47%)
   Total RT_MUSIC    size:   3540  (  0%)
   Total RT_POLY_ANIM   size: 106676  (  9%)
   Total RT_PALETTE      size:  11032  (  1%)
   Total RT_BYTECODE size: 135948  ( 11%)
   Total RT_POLY_CINEMATIC     size: 291008  ( 24%)

   As usual sounds are the most consuming assets (Quake1,Quake2 etc.....)


   memlist.bin features 146 entries :
   ==================================

   Most important part in an entry are:

   bankId          : - Give the file were the resource is.
   offset          : - How much to skip in the file before hiting the resource.
   size,packetSize : - How much to read, should we unpack what we read.



   Polygons drawing :
   =================

   Polygons can be given as:
    - a pure screenspace sequence of points: I call those screenspace polygons.
        - a list of delta to add or substract to the first vertex. I call those: objectspace polygons.

   Video :
   =======

   Q: Why 4 framebuffer ?
   A: It seems the background is generated once (like in the introduction) and stored in a framebuffer.
      Every frame the saved background is copied and new elements are drawn on top.


   Trivia :
   ========

   If you are used to RGBA 32bits per pixel framebuffer you are in for a shock:
   Another world is 16 colors palette based, making it 4bits per pixel !!

   Video generation :
   ==================

   Thank god the engine sets the palette before starting to drawing instead of after bliting.
   I would have been unable to generate screenshots otherwise.

   Memory managment :
   =================

   There is 0 malloc during the game. All resources are loaded in one big buffer (Resource::load).
   The entire buffer is "freed" at the end of a game part.


   The renderer is actually capable of Blending a new poly in the framebuffer (Video::drawLineT)


        I am almost sure that:
        _curPagePtr1 is the backbuffer
        _curPagePtr2 is the frontbuffer
        _curPagePtr3 is the background builder.


   * Why does memlist.bin uses a special state field 0xFF in order to mark the end of resources ??!
     It would have been so much easier to write the number of resources at the beginning of the code.
*/
