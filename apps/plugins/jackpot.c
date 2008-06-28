/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Copyright Kévin Ferrare based on work by Pierre Delore
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

#include "plugin.h"
#include "pluginlib_actions.h"
#include "picture.h"

PLUGIN_HEADER

const struct button_mapping* plugin_contexts[]={generic_actions};
#define NB_PICTURES 9
#define NB_SLOTS 3

#ifdef HAVE_LCD_CHARCELLS
#define PICTURE_ROTATION_STEPS 7
static unsigned char jackpot_slots_patterns[]={
    0x00, 0x0A, 0x1F, 0x1F, 0x1F, 0x0e, 0x04, /* (+00)Heart */
    0x00, 0x04, 0x0E, 0x1F, 0x1F, 0x04, 0x0E, /* (+07)Spade */
    0x00, 0x04, 0x0E, 0x1F, 0x0E, 0x04, 0x00, /* (+14)Diamond */
    0x00, 0x15, 0x0E, 0x1F, 0x0E, 0x15, 0x00, /* (+21)Club */
    0x03, 0x04, 0x0e, 0x1F, 0x1F, 0x1F, 0x0e, /* (+28)Cherry */
    0x00, 0x04, 0x04, 0x1F, 0x04, 0x0E, 0x1F, /* (+35)Cross */
    0x04, 0x0E, 0x15, 0x04, 0x0A, 0x0A, 0x11, /* (+42)Man */
    0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, /* (+49)Square */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* (+56)Empty */
    0x00, 0x0A, 0x1F, 0x1F, 0x1F, 0x0e, 0x04  /* (+63)Heart */
};
static unsigned long char_patterns[NB_SLOTS];
#define SLEEP_TIME (HZ/24)
#else /* bitmaps LCDs */

#define PICTURE_HEIGHT (BMPHEIGHT_jackpot_slots/(NB_PICTURES+1))
#if NB_SCREENS==1
#define PICTURE_ROTATION_STEPS PICTURE_HEIGHT
#else
#define REMOTE_PICTURE_HEIGHT (BMPHEIGHT_jackpot_slots_remote/(NB_PICTURES+1))
#define PICTURE_ROTATION_STEPS REMOTE_PICTURE_HEIGHT
#endif

/* FIXME: would be nice to have better graphics ... */
#include "jackpot_slots.h"
#if NB_SCREENS==2
#include "jackpot_slots_remote.h"
#endif

const struct picture jackpot_pictures[]={
    {jackpot_slots, BMPWIDTH_jackpot_slots,PICTURE_HEIGHT},
#if NB_SCREENS==2
    {jackpot_slots_remote,BMPWIDTH_jackpot_slots_remote,REMOTE_PICTURE_HEIGHT}
#endif
};

#define SLEEP_TIME (HZ/200)
#endif /* HAVE_LCD_CHARCELLS */

static const struct plugin_api* rb;

struct jackpot
{
    /* A slot can display "NB_PICTURES" pictures
       A picture is moving up, it can take PICTURE_ROTATION_STEPS
       to move a single picture completely.
       So values in slot_state are comprised between
       0 and NB_PICTURES*PICTURE_ROTATION_STEPS
     */
    int slot_state[NB_SLOTS];
    /*
       The real state of the picture in pixels on each screen
       Different from slot_state because of the synchronised
       rotation between different sized bitmaps on remote and main screen
     */
    int state_y[NB_SCREENS][NB_SLOTS];
    int money;
};

#ifdef HAVE_LCD_CHARCELLS
void patterns_init(struct screen* display)
{
    int i;
    for(i=0;i<NB_SLOTS;i++)
        char_patterns[i]=display->get_locked_pattern();
}

void patterns_deinit(struct screen* display)
{
    /* Restore the old pattern */
    int i;
    for(i=0;i<NB_SLOTS;i++)
        display->unlock_pattern(char_patterns[i]);
}
#endif /* HAVE_LCD_CHARCELLS */

/*Call when the program exit*/
void jackpot_exit(void *parameter)
{
    (void)parameter;
#ifdef HAVE_LCD_CHARCELLS
    patterns_deinit(rb->screens[SCREEN_MAIN]);
#endif /* HAVE_LCD_CHARCELLS */
}

void jackpot_init(struct jackpot* game)
{
    int i,j;
    game->money=20;
    for(i=0;i<NB_SLOTS;i++){
        game->slot_state[i]=(rb->rand()%NB_PICTURES)*PICTURE_ROTATION_STEPS;
        FOR_NB_SCREENS(j)
            game->state_y[j][i]=-1;
    }
}

int jackpot_get_result(struct jackpot* game)
{
    int i=NB_SLOTS-1;
    int multiple=1;
    int result=0;
    for(;i>=0;i--)
    {
        result+=game->slot_state[i]*multiple/PICTURE_ROTATION_STEPS;
        multiple*=10;
    }
    return(result);
}

int jackpot_get_gain(struct jackpot* game)
{
    switch (jackpot_get_result(game))
    {
        case 111 : return(20);
        case 000 : return(15);
        case 333 : return(10);
        case 222 : return(8);
        case 555 : return(5);
        case 777 : return(4);
        case 251 : return(4);
        case 510 : return(4);
        case 686 : return(3);
        case 585 : return(3);
        case 282 : return(3);
        case 484 : return(3);
        case 787 : return(2);
        case 383 : return(2);
        case 80  : return(2);
    }
    return(0);
}

void jackpot_display_slot_machine(struct jackpot* game, struct screen* display)
{
    char str[20];
    int i;
    bool changes=false;
#ifdef HAVE_LCD_CHARCELLS
    display->putc(0, 0, '[');
#else
    const struct picture* picture= &(jackpot_pictures[display->screen_type]);
    int pos_x=(display->getwidth()-NB_SLOTS*(picture->width+1))/2;
    int pos_y=(display->getheight()-(picture->height))/2;
#endif /* HAVE_LCD_CHARCELLS */
    for(i=0;i<NB_SLOTS;i++)
    {
#ifdef HAVE_LCD_CHARCELLS
        /* the only charcell lcd is 7 pixel high */
        int state_y=(game->slot_state[i]*7)/PICTURE_ROTATION_STEPS;
#else
        int state_y=
                (picture->height*game->slot_state[i])/PICTURE_ROTATION_STEPS;
#endif /* HAVE_LCD_CHARCELLS */
        int previous_state_y=game->state_y[display->screen_type][i];
        if(state_y==previous_state_y)
            continue;/*no need to update the picture
                       as it's the same as previous displayed one*/
        changes=true;
        game->state_y[display->screen_type][i]=state_y;
#ifdef HAVE_LCD_CHARCELLS
        char* current_pattern=&(jackpot_slots_patterns[state_y]);
        display->define_pattern(char_patterns[i],
                                current_pattern);
        display->putc(i+1, 0, char_patterns[i]);
#else
        vertical_picture_draw_part(display, picture, state_y, pos_x, pos_y);
        pos_x+=(picture->width+1);
#endif
    }
    if(changes){
#ifdef HAVE_LCD_CHARCELLS
        rb->snprintf(str,sizeof(str),"$%d", game->money);
        display->putc(++i, 0, ']');
        display->puts(++i, 0, str);
#else
        rb->snprintf(str,sizeof(str),"money : $%d", game->money);
        display->puts(0, 0, str);
#endif
        display->update();
    }
}


void jackpot_info_message(struct screen* display, char* message)
{
#ifdef HAVE_LCD_CHARCELLS
    display->puts_scroll(0,1,message);
#else
    int xpos, ypos;
    int message_height, message_width;
    display->getstringsize(message, &message_width, &message_height);
    xpos=(display->getwidth()-message_width)/2;
    ypos=display->getheight()-message_height;
    rb->screen_clear_area(display, 0, ypos, display->getwidth(),
                          message_height);
    display->putsxy(xpos,ypos,message);
    display->update();
#endif /* HAVE_LCD_CHARCELLS */
}

void jackpot_print_turn_result(struct jackpot* game,
                               int gain, struct screen* display)
{
    char str[20];
    if (gain==0)
    {
        jackpot_info_message(display, "None ...");
        if (game->money<=0)
            jackpot_info_message(display, "You lose...STOP to quit");
    }
    else
    {
        rb->snprintf(str,sizeof(str),"You win %d$",gain);
        jackpot_info_message(display, str);
    }
    display->update();
}

void jackpot_play_turn(struct jackpot* game)
{
    /* How many pattern? */
    int nb_turns[NB_SLOTS];
    int i,d,gain,turns_remaining=0;
    if(game->money<=0)
        return;
    game->money--;
    for(i=0;i<NB_SLOTS;i++)
    {
        nb_turns[i]=(rb->rand()%15+5)*PICTURE_ROTATION_STEPS;
        turns_remaining+=nb_turns[i];
    }
    FOR_NB_SCREENS(d)
    {
        rb->screens[d]->clear_display();
        jackpot_info_message(rb->screens[d],"Good luck");
    }
    /* Jackpot Animation */
    while(turns_remaining>0)
    {
        for(i=0;i<NB_SLOTS;i++)
        {
            if(nb_turns[i]>0)
            {
                nb_turns[i]--;
                game->slot_state[i]++;
                if(game->slot_state[i]>=PICTURE_ROTATION_STEPS*NB_PICTURES)
                    game->slot_state[i]=0;
                turns_remaining--;
            }
        }
        FOR_NB_SCREENS(d)
            jackpot_display_slot_machine(game, rb->screens[d]);
        rb->sleep(SLEEP_TIME);
    }
    gain=jackpot_get_gain(game);
    if(gain!=0)
        game->money+=gain;
    FOR_NB_SCREENS(d)
        jackpot_print_turn_result(game, gain, rb->screens[d]);
}

enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    rb = api;
    int action, i;
    struct jackpot game;
    (void)parameter;
    rb->srand(*rb->current_tick);
#ifdef HAVE_LCD_CHARCELLS
    patterns_init(rb->screens[SCREEN_MAIN]);
#endif /* HAVE_LCD_CHARCELLS */
    jackpot_init(&game);

    FOR_NB_SCREENS(i){
        rb->screens[i]->clear_display();
        jackpot_display_slot_machine(&game, rb->screens[i]);
    }
    /*Empty the event queue*/
    rb->button_clear_queue();
    while (true)
    {
        action = pluginlib_getaction(rb, TIMEOUT_BLOCK,
                                     plugin_contexts, 1);
        switch ( action )
        {
            case PLA_QUIT:
                return PLUGIN_OK;
            case PLA_FIRE:
                jackpot_play_turn(&game);
                break;

            default:
                if (rb->default_event_handler_ex(action, jackpot_exit, NULL)
                    == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
    }
    jackpot_exit(NULL);
    return PLUGIN_OK;
}
