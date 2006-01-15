/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 Pierre Delore
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"

#ifdef HAVE_LCD_CHARCELLS

/* NIM game for the player

Rules of nim game
-----------------
There are 21 matches.
Two players (you and the cpu) alternately pick a certain number of matches and the one,
who takes the last match, loses.


History:
-------
V1.0 : 2003-07-22
    First release of the game
V1.1 : 2003-07-22
    I Change the patterns definition in order to have a clean code
V1.2 : 2003-07-30
    Patch from JB that change:
    . the win and lose message
    . the BUTTON_STOP code
    . Add a test
    I suppress the exit variable
    I suppress or translates the comments which were in French
    I put min=1 at the of the main loop ( When there are 21 matches you can decide not to
     take a match. Later you are obliged to take at least one.)
*/

PLUGIN_HEADER

/*Pattern for the game*/
static unsigned char smile[]={0x00, 0x11, 0x04, 0x04, 0x00, 0x11, 0x0E};    /* :-) */
static unsigned char cry[]  ={0x00, 0x11, 0x04, 0x04, 0x00, 0x0E, 0x11};    /* :-( */
static unsigned char pattern3[]={0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15}; /*3 parts*/
static unsigned char pattern2[]={0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14}; /*2 parts*/
static unsigned char pattern1[]={0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10}; /*1 part*/

static unsigned char str[12];   /*String use to display the first line*/
static unsigned char hsmile,hcry,h1,h2; /*Handle for the new pattern*/

static bool end;    /*If true game is finished*/
static struct plugin_api* rb;


/*Display that the action it's impossible*/
static void impossible(void)
{
    rb->lcd_puts(0,1,"Impossible!");
    rb->sleep(HZ);
    return;
}

/*Display that the CPU lose :) */
static void lose(void)
{
    rb->lcd_define_pattern(hsmile,smile);
    rb->snprintf(str,sizeof(str),"You Win!!%c",hsmile);
    rb->lcd_puts(0,1,str);
    end=true;
    rb->sleep(HZ*2);
    return;
}


/* Display that the CPU win :( */
static void win(void)
{
    rb->lcd_define_pattern(hcry,cry);
    rb->snprintf(str,sizeof(str),"You Lose!!%c",hcry);
    rb->lcd_puts(0,1,str);
    end=true;
    rb->sleep(HZ*2);
    return;
}


/*Display the first line*/
static void display_first_line(int x)
{
    int i;

    rb->snprintf(str,sizeof(str),"       =%d",x);

    rb->lcd_define_pattern(h1,pattern3);
    for(i=0;i<x/3;i++)
        str[i]=h1;

    if (x%3==2)
    {
        rb->lcd_define_pattern(h2,pattern2);
        str[i]=h2;
    }
    if (x%3==1)
    {
        rb->lcd_define_pattern(h2,pattern1);
        str[i]=h2;
    }
    rb->lcd_puts(0,0,str);
}

/* Call when the program end */
static void nim_exit(void *parameter)
{
    (void)parameter;
    
    /*Restore the old pattern*/
    rb->lcd_unlock_pattern(h1);
    rb->lcd_unlock_pattern(h2);
    rb->lcd_unlock_pattern(hsmile);
    rb->lcd_unlock_pattern(hcry);

    /*Clear the screen*/
    rb->lcd_clear_display();
}

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int y,z,button;
    int x,v,min;
    bool ok;
    bool go;

    /* if you don't use the parameter, you can do like
       this to avoid the compiler warning about it */
    (void)parameter;

    /* if you are using a global api pointer, don't forget to copy it!
       otherwise you will get lovely "I04: IllInstr" errors... :-) */
    rb = api;

    /*Get the pattern handle*/
    h1=rb->lcd_get_locked_pattern();
    h2=rb->lcd_get_locked_pattern();
    hcry=rb->lcd_get_locked_pattern();
    hsmile=rb->lcd_get_locked_pattern();


    rb->splash(HZ, true, "NIM V1.2");
    rb->lcd_clear_display();

    /* Main loop */
    while (1)
    {
        /* Init */
        x=21;
        v=1;
        y=1;
        end=false;
        min=0;

        /*Empty the event queue*/
        rb->button_clear_queue();

        /* Game loop */
        while(end!=true)
        {
            do
            {
                ok=1;
                y=1;
                display_first_line(x);

                rb->snprintf(str,sizeof(str),"[%d..%d]?=%d",min,v,y);
                rb->lcd_puts(0,1,str);

                go=false;
                while (!go)
                {
                    button = rb->button_get(true);
                    switch ( button )
                    {
                        case BUTTON_STOP|BUTTON_REL:
                            go = true;
                            nim_exit(NULL);
                            return PLUGIN_OK;
                            break;

                        case BUTTON_PLAY|BUTTON_REL:
                            go=true;
                            break;

                        case BUTTON_LEFT|BUTTON_REL:
                            go=false;
                            if (y>min)
                                y--;
                            break;

                        case BUTTON_RIGHT|BUTTON_REL:
                            go=false;
                            if (y<v)
                                y++;
                            break;

                        default:
                            if (rb->default_event_handler_ex(button, nim_exit,
                                                    NULL) == SYS_USB_CONNECTED)
                                return PLUGIN_USB_CONNECTED;
                            break;
                    }
                    display_first_line(x);
                    rb->snprintf(str,sizeof(str),"[%d..%d]?=%d",min,v,y);
                    rb->lcd_puts(0,1,str);
                }

                if ( (y==0) && (x<21))
                {
                    impossible();
                    ok=false;
                }
                else
                {
                    if (y!=0) /*If y=0 and x=21 jump to CPU code */
                    {
                        if ((y>v) || (y>x))
                        {
                            impossible();
                            ok=false;
                        }
                        if (y-x==0)
                            win();
                        else
                        {
                            v=y*2;
                            x-=y;
                        }
                    }
                }
            }
            while (ok==false);

            display_first_line(x);

            /*CPU*/
            if (x==1)
                lose();
            else
                if (x==2)
                    win();
            y=0;
            if (end==false)
            {
                for (z=v;z>=1;z--)
                {
                    if (x-z==1)
                        y=z;
                }
                if (y<=0)
                {
                    for(z=v;z>=1;z--)
                    {
                        if(x-(z*3)==2)
                            y=z;
                    }
                    if ((y==0) && (x>14))
                        y=v;
                    if (y==0)
                        y=1;
                }
                v=y*2;
                x-=y;
                rb->snprintf(str,sizeof(str),"I take=%d",y);
                rb->lcd_puts(0,1,str);
                rb->sleep(HZ);
            }
            if ((x==1)&&(!end))
                win();
            min=1;
        }
    }
    nim_exit(NULL);
    return PLUGIN_OK;
}
#endif
