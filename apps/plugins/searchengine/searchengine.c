/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id
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
    }
    if (size + 4 > audio_buffer_free)
        return 0;
    alloc = audio_bufferpointer;
    audio_bufferpointer += size + 4;
    audio_buffer_free -= size + 4;
    return alloc;
}

void setmallocpos(void *pointer) 
{
    audio_bufferpointer = pointer;
    audio_buffer_free = audio_bufferpointer - audio_bufferbase;
}

struct token tokenstream[10];

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    unsigned char *result,buf[500];
    /* this macro should be called as the first thing you do in the plugin.
       it test that the api version and model the plugin was compiled for
       matches the machine it is running on */
    TEST_PLUGIN_API(api);

    (void)parameter;

    /* if you are using a global api pointer, don't forget to copy it!
       otherwise you will get lovely "I04: IllInstr" errors... :-) */
    rb = api;
    
    audio_bufferbase=audio_bufferpointer=0;
    audio_buffer_free=0;

    /* now go ahead and have fun! */
    rb->splash(HZ*2, true, "SearchEngine v0.1"); 
    tokenstream[0].kind=TOKEN_NUMIDENTIFIER;
    tokenstream[0].intvalue=INTVALUE_YEAR;
    tokenstream[1].kind=TOKEN_GTE;
    tokenstream[2].kind=TOKEN_NUM;
    tokenstream[2].intvalue=1980;
    tokenstream[3].kind=TOKEN_AND;
    tokenstream[4].kind=TOKEN_NUMIDENTIFIER;
    tokenstream[4].intvalue=INTVALUE_YEAR;
    tokenstream[5].kind=TOKEN_LT;
    tokenstream[6].kind=TOKEN_NUM;
    tokenstream[6].intvalue=1990;
    tokenstream[7].kind=TOKEN_EOF;
    result=parse(tokenstream);
    rb->snprintf(buf,250,"Retval: 0x%x",result);
    PUTS(buf);
    if(result!=0) {
	int fd=rb->open("/search.m3u", O_WRONLY|O_CREAT|O_TRUNC);
	int i;
	for(i=0;i<rb->tagdbheader->filecount;i++)
	  if(result[i])
	    rb->fdprintf(fd,"%s\n",getfilename(i));
/*	rb->write(fd,result,rb->tagdbheader->filecount);*/
    	rb->close(fd);
    }
    rb->sleep(HZ*10);
    return PLUGIN_OK;
}
