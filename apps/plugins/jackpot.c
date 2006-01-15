/***************************************************************************
 *           __________        __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                   \/     \/     \/    \/         \/
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
/*
History:
* V1.0: 21/07/03
 First version with a dirty definition of the patterns.
* V1.1: 24/07/03
 Clean definition of the patterns. 
 Init message change
*/
                    
#include "plugin.h"

#ifdef HAVE_LCD_CHARCELLS

PLUGIN_HEADER

/* Jackpot game for the player */

static unsigned char pattern[]={
    0x00, 0x0A, 0x1F, 0x1F, 0x1F, 0x0e, 0x04, /* (+00)Coeur */
    0x00, 0x04, 0x0E, 0x1F, 0x1F, 0x04, 0x0E, /* (+07)Pique */
    0x00, 0x04, 0x0E, 0x1F, 0x0E, 0x04, 0x00, /* (+14)Carreau */
    0x00, 0x15, 0x0E, 0x1F, 0x0E, 0x15, 0x00, /* (+21)Treffle */
    0x03, 0x04, 0x0e, 0x1F, 0x1F, 0x1F, 0x0e, /* (+28)Cerise */
    0x00, 0x04, 0x04, 0x1F, 0x04, 0x0E, 0x1F, /* (+35)Carreau */
    0x04, 0x0E, 0x15, 0x04, 0x0A, 0x0A, 0x11, /* (+42)Homme */
    0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, /* (+49)Carre */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* (+56)Empty */
    0x00, 0x0A, 0x1F, 0x1F, 0x1F, 0x0e, 0x04 /* (+63)Coeur */
};

static unsigned char str[12]; /*Containt the first line*/
static unsigned char h1,h2,h3; /*Handle for the pattern*/

/* here is a global api struct pointer. while not strictly necessary,
   it's nice not to have to pass the api pointer in all function calls
   in the plugin */
static struct plugin_api* rb;

/*Display the first line*/
static void display_first_line(int g)
{
    rb->snprintf(str,sizeof(str),"[   ]$%d",g);
    rb->lcd_puts(0,0,str);

    rb->lcd_putc(1,0, h1);
    rb->lcd_putc(2,0, h2);
    rb->lcd_putc(3,0, h3);
}

/*Call when the program exit*/
static void jackpot_exit(void *parameter)
{
    (void)parameter;
    
    /* Restore the old pattern (i don't find another way to do this. Any
       idea?) */
    rb->lcd_unlock_pattern(h1);
    rb->lcd_unlock_pattern(h2);
    rb->lcd_unlock_pattern(h3);

    /* Clear the screen */
    rb->lcd_clear_display();
}


/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int i,button,w,j;
    int s[3];
    int n[3];
    int g=20;
    bool exit=false;
    bool go;

    /* if you don't use the parameter, you can do like
       this to avoid the compiler warning about it */
    (void)parameter;

    /* if you are using a global api pointer, don't forget to copy it!
       otherwise you will get lovely "I04: IllInstr" errors... :-) */
    rb = api;
    rb->srand(*rb->current_tick);

    /*Get the pattern handle*/
    h1=rb->lcd_get_locked_pattern();
    h2=rb->lcd_get_locked_pattern();
    h3=rb->lcd_get_locked_pattern();

    /*Init message*/    
    rb->lcd_define_pattern(h1, pattern);
    rb->lcd_define_pattern(h2, pattern+7);
    rb->lcd_define_pattern(h3, pattern+28);
    rb->snprintf(str,sizeof(str),"%c%cJackpot%c%c",h1,h2,h2,h1);
    rb->lcd_puts(0,0,str);
    rb->snprintf(str,sizeof(str)," %c V1.1  %c",h3,h3);
    rb->lcd_puts(0,1,str);
    rb->sleep(HZ*2);
    rb->lcd_clear_display();

    /*First display*/
    rb->snprintf(str,sizeof(str),"[   ]$%d",g);
    rb->lcd_puts(0,0,str);
    rb->lcd_puts_scroll(0,1,"PLAY to begin");

    /*Empty the event queue*/
    rb->button_clear_queue();

    /* Define the start pattern */
    s[0]=(rb->rand()%9)*7;
    s[1]=(rb->rand()%9)*7;
    s[2]=(rb->rand()%9)*7;

    /*Main loop*/
    while (!exit)
    {
        /*Keyboard loop*/
        go=false;
        while (!go)
        {
            button = rb->button_get(true);
            switch ( button )
            {
                case BUTTON_STOP|BUTTON_REL:
                    exit = true;
                    go = true;
                    break;

                case BUTTON_PLAY|BUTTON_REL:
                    exit = false;
                    if (g>0)
                    {
                        go = true;
                        g--;
                    }
                    break;

                default:
                    if (rb->default_event_handler_ex(button, jackpot_exit,
                                                    NULL) == SYS_USB_CONNECTED)
                        return PLUGIN_USB_CONNECTED;
                    break;
            }
        }

        /*Clear the Second line*/
        rb->lcd_puts_scroll(0,1,"Good luck");

        /*GO !!!!*/
        if ( go && !exit )
        {
            /* How many pattern? */
            n[0]=(rb->rand()%15+5)*7;
            n[1]=(rb->rand()%15+5)*7;
            n[2]=(rb->rand()%15+5)*7;

            display_first_line(g);
            /* Jackpot Animation */
            while((n[0]>=0) || (n[1]>=0) || (n[2]>=0))
            {
                if (n[0]>=0)
                    rb->lcd_define_pattern(h1, pattern+s[0]);
                if (n[1]>=0)
                    rb->lcd_define_pattern(h2, pattern+s[1]);
                if (n[2]>=0)
                    rb->lcd_define_pattern(h3, pattern+s[2]);
                rb->sleep(HZ/24);
                rb->lcd_putc(1,0, h1);
                rb->lcd_putc(2,0, h2);
                rb->lcd_putc(3,0, h3);
                for(i=0;i<3;i++)
                {
                    if (n[i]>=0)
                    {
                        n[i]--;
                        s[i]++;
                        if (s[i]>=63)
                            s[i]=0;
                    }
                }
            }

            /* You won? */
            s[0]--;
            s[1]--;
            s[2]--;
            w=(s[0]/7)*100+(s[1]/7)*10+(s[2]/7);

            j=0;
            switch (w)
            {
                case 111 :
                    j=20;
                    break;
                case 000 :
                    j=15;
                    break;
                case 333 :
                    j=10;
                    break;
                case 222 :
                    j=8;
                    break;
                case 555 :
                    j=5;
                    break;
                case 777 :
                    j=4;
                    break;
                case 251 :
                    j=4;
                    break;
                case 510 :
                    j=4;
                    break;
                case 686 :
                    j=3;
                    break;
                case 585 :
                    j=3;
                    break;
                case 282 :
                    j=3;
                    break;
                case 484 :
                    j=3;
                    break;
                case 787 :
                    j=2;
                    break;
                case 383 :
                    j=2;
                    break;
                case 80 :
                    j=2;
                    break;
            }
            if (j==0)
            {
                rb->lcd_puts(0,1,"None...");
                if (g<=0)
                    rb->lcd_puts_scroll(0,1,"You lose...STOP to quit");
            }
            else
            {
                g+=j;
                display_first_line(g);
                rb->snprintf(str,sizeof(str),"You win %d$",j);
                rb->lcd_puts(0,1,str);
            }
        }
    }

    /* This is the end */
    jackpot_exit(NULL);
    /* Bye */
    return PLUGIN_OK;
}

#endif
