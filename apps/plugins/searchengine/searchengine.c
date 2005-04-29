/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Michiel van der Kolk 
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "searchengine.h"
#include "parser.h"
#include "token.h"
#include "dbinterface.h"

void *audio_bufferbase;
void *audio_bufferpointer;
unsigned int audio_buffer_free;
struct plugin_api* rb;
int w, h, y;

void *my_malloc(size_t size)
{
    void *alloc;

    if (!audio_bufferbase)
    {
        audio_bufferbase = audio_bufferpointer
            = rb->plugin_get_audio_buffer(&audio_buffer_free);
        audio_bufferpointer+=3;
        audio_bufferpointer=(void *)(((int)audio_bufferpointer)&~3);
        audio_buffer_free-=audio_bufferpointer-audio_bufferbase;
    }
    if (size + 4 > audio_buffer_free)
        return 0;
    alloc = audio_bufferpointer;
    audio_bufferpointer +=(size+3)&~3; // alignment
    audio_buffer_free -= (size+3)&~3;
    return alloc;
}

void setmallocpos(void *pointer) 
{
    audio_bufferpointer = pointer;
    audio_buffer_free = audio_bufferpointer - audio_bufferbase;
}

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    unsigned char *result,buf[500];
    int parsefd,hits;
    /* this macro should be called as the first thing you do in the plugin.
       it test that the api version and model the plugin was compiled for
       matches the machine it is running on */
    TEST_PLUGIN_API(api);

    /* if you are using a global api pointer, don't forget to copy it!
       otherwise you will get lovely "I04: IllInstr" errors... :-) */
    rb = api;
    
    audio_bufferbase=audio_bufferpointer=0;
    audio_buffer_free=0;

    /* now go ahead and have fun! */
    PUTS("SearchEngine v0.1");
    parsefd=rb->open(parameter,O_RDONLY);
    if(parsefd<0) {
        rb->splash(2*HZ,true,"Unable to open search tokenstream");
        return PLUGIN_ERROR;    
    }
    result=parse(parsefd);
    rb->snprintf(buf,250,"Retval: 0x%x",result);
    PUTS(buf);
    rb->close(parsefd);
    hits=0;
    if(result!=0) {
        int fd=rb->open("/search.m3u", O_WRONLY|O_CREAT|O_TRUNC);
        int i;
        for(i=0;i<rb->tagdbheader->filecount;i++)
            if(result[i]) {
                hits++;
                rb->fdprintf(fd,"%s\n",getfilename(i));
            }
        rb->close(fd);
    }
    rb->snprintf(buf,250,"Hits: %d",hits);
    rb->splash(HZ*3,true,buf);
    return PLUGIN_OK;
}
