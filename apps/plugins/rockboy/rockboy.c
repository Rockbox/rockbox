/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Gameboy emulator based on gnuboy
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "loader.h"
#include "rockmacros.h"

#if MEM <= 8 && !defined(SIMULATOR)
/* On archos this is loaded as an overlay */

/* These are defined in the linker script */
extern unsigned char ovl_start_addr[];
extern unsigned char ovl_end_addr[];

const struct {
    unsigned long magic;
    unsigned char *start_addr;
    unsigned char *end_addr;
    enum plugin_status (*entry_point)(struct plugin_api*, void*);
} header __attribute__ ((section (".header"))) = {
    0x524f564c, /* ROVL */
    ovl_start_addr, ovl_end_addr, plugin_start
};
#endif

#ifdef USE_IRAM
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
#endif

/* here is a global api struct pointer. while not strictly necessary,
   it's nice not to have to pass the api pointer in all function calls
   in the plugin */
struct plugin_api* rb;
int shut,cleanshut;
char *errormsg;
int gnuboy_main(char *rom);

void die(char *message, ...)
{
    shut=1;
    errormsg=message;
}

void *mp3_bufferbase;
void *mp3_bufferpointer;
unsigned int mp3_buffer_free;

void *my_malloc(size_t size)
{
    void *alloc;

    if (!mp3_bufferbase)
    {
        mp3_bufferbase = mp3_bufferpointer
                       = rb->plugin_get_mp3_buffer(&mp3_buffer_free);
#if MEM <= 8 && !defined(SIMULATOR)
        /* loaded as an overlay, protect from overwriting ourselves */
        if ((unsigned)(ovl_start_addr - (unsigned char *)mp3_bufferbase) 
            < mp3_buffer_free)
            mp3_buffer_free = ovl_start_addr - (unsigned char *)mp3_bufferbase;
#endif
    }
    if (size + 4 > mp3_buffer_free)
        return 0;
    alloc = mp3_bufferpointer;
    mp3_bufferpointer += size + 4;
    mp3_buffer_free -= size + 4;
    return alloc;
}

void setmallocpos(void *pointer) 
{
    mp3_bufferpointer = pointer;
    mp3_buffer_free = mp3_bufferpointer - mp3_bufferbase;
}

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    /* this macro should be called as the first thing you do in the plugin.
       it test that the api version and model the plugin was compiled for
       matches the machine it is running on */
    TEST_PLUGIN_API(api);

    /* if you are using a global api pointer, don't forget to copy it!
       otherwise you will get lovely "I04: IllInstr" errors... :-) */
    rb = api;
#ifdef USE_IRAM
          memcpy(iramstart, iramcopy, iramend-iramstart);
#endif
    shut=0;
    cleanshut=0;
    mp3_bufferbase=mp3_bufferpointer=0;
    mp3_buffer_free=0;

    /* now go ahead and have fun! */
    /* rb->splash(HZ*2, true, "Rockboy v0.3"); */
    /* rb->lcd_clear_display(); */
    gnuboy_main(parameter);

    if(shut&&!cleanshut) {
        rb->splash(HZ*2, true, errormsg);
        return PLUGIN_ERROR;
    }

    rb->splash(HZ*2, true, "Shutting down.. byebye ^^");

    cleanup();

    return PLUGIN_OK;
}
