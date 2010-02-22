/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Will Robertson
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
#include "lib/playback_control.h"
#include "lib/display_text.h"
#include "pluginbitmaps/superdom_boarditems.h"
PLUGIN_HEADER

extern const fb_data superdom_boarditems[];
char buf[255];

#define COLOUR_DARK 0
#define COLOUR_LIGHT 1

#define MARGIN 5

#if (LCD_DEPTH == 16)
#define MY_BITMAP_PART   rb->lcd_bitmap_transparent_part
#else
#define MY_BITMAP_PART   rb->lcd_mono_bitmap_part
#endif

#if LCD_WIDTH > LCD_HEIGHT
#define BOX_WIDTH ((LCD_WIDTH-(MARGIN*2))/10)
#define BOX_HEIGHT ((BOX_WIDTH*2)/3)

#else
#define BOX_HEIGHT ((LCD_HEIGHT-(MARGIN*2)-15)/10)
#define BOX_WIDTH ((BOX_HEIGHT*2)/3)

#endif

/* NUM_BOX HEIGHT and WIDTH are used for the number pad in the game.  The height
 *  calculation includes spacing for the text placed above and below the number
 *  pad (it divides by 6 instead of just 4).  The width calculation gives extra
 *  spacing on the sides of the pad too (divides by 5 instead of 3).
 */
#define NUM_BOX_HEIGHT  (LCD_HEIGHT/6)
#define NUM_BOX_WIDTH   (LCD_WIDTH/5)

#define NUM_MARGIN_X    (LCD_WIDTH-3*NUM_BOX_WIDTH)/2
#define NUM_MARGIN_Y    (LCD_HEIGHT-4*NUM_BOX_HEIGHT)/2

/* These parameters define the piece image dimensions, Stride is the total width
 *  of the bitmap.
 */
#define ICON_STRIDE     STRIDE(SCREEN_MAIN, BMPWIDTH_superdom_boarditems, BMPHEIGHT_superdom_boarditems)
#define ICON_HEIGHT     (BMPHEIGHT_superdom_boarditems/6)
#define ICON_WIDTH      (BMPWIDTH_superdom_boarditems/2)

#if (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
    (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define SUPERDOM_OK BUTTON_SELECT
#define SUPERDOM_CANCEL BUTTON_MENU
#define SUPERDOM_LEFT BUTTON_LEFT
#define SUPERDOM_RIGHT BUTTON_RIGHT
#define IPOD_STYLE

#elif CONFIG_KEYPAD == IRIVER_H300_PAD || CONFIG_KEYPAD == IRIVER_H100_PAD
#define SUPERDOM_OK BUTTON_SELECT
#define SUPERDOM_LEFT BUTTON_LEFT
#define SUPERDOM_RIGHT BUTTON_RIGHT
#define SUPERDOM_UP BUTTON_UP
#define SUPERDOM_DOWN BUTTON_DOWN
#define SUPERDOM_CANCEL BUTTON_OFF

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define SUPERDOM_OK BUTTON_SELECT
#define SUPERDOM_LEFT BUTTON_LEFT
#define SUPERDOM_RIGHT BUTTON_RIGHT
#define SUPERDOM_UP BUTTON_UP
#define SUPERDOM_DOWN BUTTON_DOWN
#define SUPERDOM_CANCEL BUTTON_REC

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define SUPERDOM_OK BUTTON_RIGHT
#define SUPERDOM_UP BUTTON_SCROLL_UP
#define SUPERDOM_DOWN BUTTON_SCROLL_DOWN
#define SUPERDOM_CANCEL BUTTON_LEFT

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define SUPERDOM_OK BUTTON_SELECT
#define SUPERDOM_UP BUTTON_UP
#define SUPERDOM_DOWN BUTTON_DOWN
#define SUPERDOM_LEFT BUTTON_LEFT
#define SUPERDOM_RIGHT BUTTON_RIGHT
#define SUPERDOM_CANCEL BUTTON_POWER

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define SUPERDOM_OK BUTTON_SELECT
#define SUPERDOM_UP BUTTON_SCROLL_BACK
#define SUPERDOM_DOWN BUTTON_SCROLL_FWD
#define SUPERDOM_LEFT BUTTON_LEFT
#define SUPERDOM_RIGHT BUTTON_RIGHT
#define SUPERDOM_CANCEL BUTTON_POWER

#elif CONFIG_KEYPAD == SANSA_FUZE_PAD
#define SUPERDOM_OK BUTTON_SELECT
#define SUPERDOM_UP BUTTON_SCROLL_BACK
#define SUPERDOM_DOWN BUTTON_SCROLL_FWD
#define SUPERDOM_LEFT BUTTON_LEFT
#define SUPERDOM_RIGHT BUTTON_RIGHT
#define SUPERDOM_CANCEL (BUTTON_HOME|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define SUPERDOM_OK BUTTON_SELECT
#define SUPERDOM_UP BUTTON_UP
#define SUPERDOM_DOWN BUTTON_DOWN
#define SUPERDOM_LEFT BUTTON_LEFT
#define SUPERDOM_RIGHT BUTTON_RIGHT
#define SUPERDOM_CANCEL BUTTON_BACK

#elif CONFIG_KEYPAD == COWON_D2_PAD
#define SUPERDOM_CANCEL BUTTON_POWER

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define SUPERDOM_OK BUTTON_SELECT
#define SUPERDOM_UP BUTTON_UP
#define SUPERDOM_DOWN BUTTON_DOWN
#define SUPERDOM_LEFT BUTTON_LEFT
#define SUPERDOM_RIGHT BUTTON_RIGHT
#define SUPERDOM_CANCEL BUTTON_BACK

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#define SUPERDOM_OK BUTTON_PLAY
#define SUPERDOM_UP BUTTON_UP
#define SUPERDOM_DOWN BUTTON_DOWN
#define SUPERDOM_LEFT BUTTON_PREV
#define SUPERDOM_RIGHT BUTTON_NEXT
#define SUPERDOM_CANCEL BUTTON_LEFT

#elif (CONFIG_KEYPAD == ONDAVX747_PAD) || (CONFIG_KEYPAD == MROBE500_PAD)
#define SUPERDOM_CANCEL BUTTON_POWER

#elif CONFIG_KEYPAD == SAMSUNG_YH_PAD
#define SUPERDOM_OK     BUTTON_PLAY
#define SUPERDOM_UP     BUTTON_UP
#define SUPERDOM_DOWN   BUTTON_DOWN
#define SUPERDOM_LEFT   BUTTON_LEFT
#define SUPERDOM_RIGHT  BUTTON_RIGHT
#define SUPERDOM_CANCEL BUTTON_REW

#elif CONFIG_KEYPAD == PBELL_VIBE500_PAD
#define SUPERDOM_OK     BUTTON_OK
#define SUPERDOM_UP     BUTTON_UP
#define SUPERDOM_DOWN   BUTTON_DOWN
#define SUPERDOM_LEFT   BUTTON_PREV
#define SUPERDOM_RIGHT  BUTTON_NEXT
#define SUPERDOM_CANCEL BUTTON_CANCEL

#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef SUPERDOM_OK
#define SUPERDOM_OK     BUTTON_CENTER
#endif
#ifndef SUPERDOM_UP
#define SUPERDOM_UP     BUTTON_TOPMIDDLE
#endif
#ifndef SUPERDOM_LEFT
#define SUPERDOM_LEFT   BUTTON_MIDLEFT
#endif
#ifndef SUPERDOM_RIGHT
#define SUPERDOM_RIGHT  BUTTON_MIDRIGHT
#endif
#ifndef SUPERDOM_DOWN
#define SUPERDOM_DOWN   BUTTON_BOTTOMMIDDLE
#endif
#ifndef SUPERDOM_CANCEL
#define SUPERDOM_CANCEL BUTTON_TOPLEFT
#endif
#endif

enum {
    RET_VAL_OK,
    RET_VAL_USB,
    RET_VAL_QUIT_ERR, /* quit or error */
};

void gen_interest(void);
void init_resources(void);
int select_square(void);
void update_score(void);
void gen_resources(void);
void draw_cursor(void);
void draw_board(void);

struct tile{
    signed int colour; /* -1 = Unset */
    bool tank;
    bool plane;
    bool nuke;
    bool ind;
    bool farm;
    int men;
};

struct resources {
    int cash;
    int food;
    int farms;
    int inds;
    int men;
    int tanks;
    int planes;
    int nukes;
    int bank;
    int moves;
};

struct settings {
    int compstartfarms;
    int compstartinds;
    int humanstartfarms;
    int humanstartinds;
    int startcash;
    int startfood;
    int movesperturn;
} superdom_settings;

struct resources humanres;
struct resources compres;
enum { GS_PROD, GS_MOVE, GS_WAR } gamestate;

struct cursor{
    int x;
    int y;
} cursor;

struct tile board[12][12];

void init_board(void) {
    int i,j;
    rb->srand(*rb->current_tick);
    for(i=0;i<12;i++) {  /* Hopefully about 50% each colour */
        for(j=0;j<12;j++) {
            if((i<1)||(j<1)||(i>10)||(j>10))
                board[i][j].colour = -1;   /* Unset */
            else
                board[i][j].colour = rb->rand()%2;
            board[i][j].tank = false;
            board[i][j].plane = false;
            board[i][j].nuke = false;
            board[i][j].ind = false;
            board[i][j].farm = false;
            board[i][j].men = 0;
        }
    }

    while(compres.farms < superdom_settings.compstartfarms) {
        i = rb->rand()%10 + 1;
        j = rb->rand()%10 + 1;
        if((board[i][j].colour == COLOUR_DARK) && (board[i][j].farm == false)) {
            board[i][j].farm = true;
            compres.farms++;
        }
    }
    while(compres.inds < superdom_settings.compstartinds) {
        i = rb->rand()%10 + 1;
        j = rb->rand()%10 + 1;
        if((board[i][j].colour == COLOUR_DARK) && (board[i][j].ind == false)) {
            board[i][j].ind = true;
            compres.inds++;
        }
    }
    while(humanres.farms < superdom_settings.humanstartfarms) {
        i = rb->rand()%10 + 1;
        j = rb->rand()%10 + 1;
        if((board[i][j].colour == COLOUR_LIGHT)&&(board[i][j].farm == false)) {
            board[i][j].farm = true;
            humanres.farms++;
        }
    }
    while(humanres.inds < superdom_settings.humanstartinds) {
        i = rb->rand()%10 + 1;
        j = rb->rand()%10 + 1;
        if((board[i][j].colour == COLOUR_LIGHT) && (board[i][j].ind == false)) {
            board[i][j].ind = true;
            humanres.inds++;
        }
    }
}

void draw_board(void) {
    int i,j;
    rb->lcd_clear_display();
    for(i=1;i<11;i++) {
        for(j=1;j<11;j++) {
            if(board[i][j].colour == COLOUR_DARK) {
                rb->lcd_set_foreground(LCD_DARKGRAY);
            } else {
                rb->lcd_set_foreground(LCD_LIGHTGRAY);
            }
            rb->lcd_fillrect(MARGIN+(BOX_WIDTH*(i-1)), 
                            MARGIN+(BOX_HEIGHT*(j-1)), BOX_WIDTH, 
                            BOX_HEIGHT);
#if LCD_DEPTH != 16
            rb->lcd_set_drawmode(DRMODE_BG | DRMODE_INVERSEVID); 
#endif
            if(board[i][j].ind) {
                MY_BITMAP_PART(superdom_boarditems,
                                board[i][j].colour?ICON_WIDTH:0, 0, ICON_STRIDE, 
#if LCD_WIDTH > LCD_HEIGHT
                                MARGIN+(BOX_WIDTH*(i-1))+1, 
                                MARGIN+(BOX_HEIGHT*(j-1))+ICON_HEIGHT+1, 
#else
                                MARGIN+(BOX_WIDTH*(i-1))+1+ICON_WIDTH,
                                MARGIN+(BOX_HEIGHT*(j-1))+1,
#endif
                                ICON_WIDTH, ICON_HEIGHT);
            }
            if(board[i][j].farm) {
                MY_BITMAP_PART(superdom_boarditems,
                                board[i][j].colour?ICON_WIDTH:0, ICON_HEIGHT, 
                                ICON_STRIDE, MARGIN+(BOX_WIDTH*(i-1))+1, 
                                MARGIN+(BOX_HEIGHT*(j-1))+1, 
                                ICON_WIDTH, ICON_HEIGHT);
            }
            if(board[i][j].tank) {
                MY_BITMAP_PART(superdom_boarditems,
                                board[i][j].colour?ICON_WIDTH:0, ICON_HEIGHT*2,
                                ICON_STRIDE, MARGIN+(BOX_WIDTH*(i-1))+ICON_WIDTH+1, 
                                MARGIN+(BOX_HEIGHT*(j-1))+ICON_HEIGHT+1, 
                                ICON_WIDTH, ICON_HEIGHT);
            }
            if(board[i][j].men) {
                MY_BITMAP_PART(superdom_boarditems,
                                board[i][j].colour?ICON_WIDTH:0, ICON_HEIGHT*3,
#if LCD_WIDTH > LCD_HEIGHT
                                ICON_STRIDE, MARGIN+(BOX_WIDTH*(i-1))+ICON_WIDTH+1,
                                MARGIN+(BOX_HEIGHT*(j-1))+1, 
#else
                                ICON_STRIDE, MARGIN+(BOX_WIDTH*(i-1))+1,
                                MARGIN+(BOX_HEIGHT*(j-1))+1+ICON_HEIGHT,
#endif
                                ICON_WIDTH, ICON_HEIGHT);
            }
            if(board[i][j].plane) {
                MY_BITMAP_PART(superdom_boarditems,
                                board[i][j].colour?ICON_WIDTH:0, ICON_HEIGHT*4,
#if LCD_WIDTH > LCD_HEIGHT
                                ICON_STRIDE,MARGIN+(BOX_WIDTH*(i-1))+ICON_WIDTH*2+1,
                                MARGIN+(BOX_HEIGHT*(j-1))+ICON_HEIGHT+1,
#else
                                ICON_STRIDE,MARGIN+(BOX_WIDTH*(i-1))+ICON_WIDTH+1,
                                MARGIN+(BOX_HEIGHT*(j-1))+ICON_HEIGHT*2+1,
#endif
                                ICON_WIDTH, ICON_HEIGHT);
            }
            if(board[i][j].nuke) {
                MY_BITMAP_PART(superdom_boarditems,
                                board[i][j].colour?ICON_WIDTH:0, ICON_HEIGHT*5,
#if LCD_WIDTH > LCD_HEIGHT
                                ICON_STRIDE,MARGIN+(BOX_WIDTH*(i-1))+ICON_WIDTH*2+1,
                                MARGIN+(BOX_HEIGHT*(j-1))+1, 
#else
                                ICON_STRIDE,MARGIN+(BOX_WIDTH*(i-1))+1,
                                MARGIN+(BOX_HEIGHT*(j-1))+ICON_HEIGHT*2+1,
#endif
                                ICON_WIDTH, ICON_HEIGHT);
            }
#if LCD_DEPTH != 16
            rb->lcd_set_drawmode(DRMODE_SOLID);
#endif
        }
    }
    rb->lcd_set_foreground(LCD_BLACK);
    for(i=0;i<=10;i++) { /* Draw Horizontal lines */
        rb->lcd_hline(MARGIN, MARGIN+(BOX_WIDTH*10), MARGIN+(BOX_HEIGHT*i));
    }
    for(i=0;i<=10;i++) { /* Draw Vertical lines */
        rb->lcd_vline(MARGIN+(BOX_WIDTH*i), MARGIN, MARGIN+(BOX_HEIGHT*10));
    }
    rb->lcd_update();
}

int calc_strength(int colour, int x, int y) {
    int a, b, score=0;
    for (a = -1; a < 2; a++) {
        for (b = -1; b < 2; b++) {
            if ((b == 0 || a == 0) &&
                (board[x + a][y + b].colour == colour)) {
                score += 10;
                if(board[x + a][y + b].tank || board[x + a][y + b].farm)
                    score += 30;
                if(board[x + a][y + b].plane || board[x + a][y + b].ind)
                    score += 40;
                if(board[x + a][y + b].nuke)
                    score += 20;
                if(board[x + a][y + b].men)
                    score += (board[x + a][y + b].men*133/1000);
            }
        }
    }
    return score;
}

void gen_interest(void) {
    /* Interest should be around 10% */
    rb->srand(*rb->current_tick);
    int interest = 7+rb->rand()%6;
    humanres.bank = humanres.bank+(interest*humanres.bank/100);
    compres.bank = compres.bank+(interest*compres.bank/100);
}

void draw_cursor(void) {
    rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
    rb->lcd_fillrect(MARGIN+((cursor.x-1)*BOX_WIDTH), 
                  MARGIN+((cursor.y-1)*BOX_HEIGHT), BOX_WIDTH+1, BOX_HEIGHT+1);
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_update();
}

void gen_resources(void) {
    int inccash = 0;
    int incfood = 0;
    int ratecash = 0;
    int ratefood = 0;
    int i;
    gen_interest();
    rb->srand(*rb->current_tick);
    /* Generate Human's resources */
        for(i=0;i<humanres.inds;i++) {
            inccash += (300+rb->rand()%200);
        }
        for(i=0;i<humanres.farms;i++) {
            incfood += (200+rb->rand()%200);
        }
        if(humanres.inds)
            ratecash = inccash/humanres.inds;
        if(humanres.farms)
            ratefood = incfood/humanres.farms;
        if(ratecash > 450) {
            if(ratefood > 350) {
                rb->splash(HZ*2, "Patriotism sweeps the land, all production" 
                                " is up this year!");
            } else {
                rb->splash(HZ*2, "Factories working at maximum efficiency," 
                                " cash production up this year!");
            }
        } else if(ratecash > 350) {
            if(ratefood > 350) {
                rb->splash(HZ*2, "Record crop harvest this year!");
            } else if(ratefood > 250) {
                rb->splash(HZ*2, "Production continues as normal");
            } else {
                rb->splash(HZ*2, "Spoilage of crops leads to reduced farm" 
                                " output this  year");
            }
        } else {
            if(ratefood > 350) {
                rb->splash(HZ*2, "Record crop harvest this year!");
            } else if(ratefood > 250) {
                rb->splash(HZ*2, "Factory unions introduced. Industrial" 
                                " production is down this year.");
            } else {
                rb->splash(HZ*2, "Internet created. All production is down" 
                                " due to time wasted.");
            }
        }
        humanres.cash += inccash;
        humanres.food += incfood;

    /* Generate Computer's resources */
        inccash = 0;
        incfood = 0;
        for(i=0;i<compres.inds;i++) {
            inccash += (300+rb->rand()%200);
        }
        for(i=0;i<compres.farms;i++) {
            incfood += (200+rb->rand()%200);
        }
        compres.cash += inccash;
        compres.food += incfood;
}

void update_score(void) {
    int strength;
    rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_set_drawmode(DRMODE_BG|DRMODE_INVERSEVID);
    rb->lcd_fillrect(5,LCD_HEIGHT-20,105,20);
    rb->lcd_set_drawmode(DRMODE_SOLID);
    strength = calc_strength(COLOUR_LIGHT, cursor.x, cursor.y);
    rb->snprintf(buf, sizeof(buf), "Your power: %d.%d", 
                    strength/10, strength%10);
    rb->lcd_putsxy(5,LCD_HEIGHT-20, buf);
    strength = calc_strength(COLOUR_DARK, cursor.x, cursor.y);
    rb->snprintf(buf, sizeof(buf), "Comp power: %d.%d", 
                    strength/10, strength%10);
    rb->lcd_putsxy(5,LCD_HEIGHT-10, buf);
    rb->lcd_setfont(FONT_UI);
}

int settings_menu(void) {
    int selection = 0;

    MENUITEM_STRINGLIST(menu, "Super Domination Settings", NULL,
                    "Computer starting farms", "Computer starting factories",
                    "Human starting farms", "Human starting factories",
                    "Starting cash", "Starting food", "Moves per turn");

    while(1) {
        switch(rb->do_menu(&menu, &selection, NULL, false)) {
        case 0:
            rb->set_int("Computer starting farms", "", UNIT_INT, 
                            &superdom_settings.compstartfarms, NULL, 
                            1, 0, 5, NULL);
            break;
        case 1:
            rb->set_int("Computer starting factories", "", UNIT_INT, 
                            &superdom_settings.compstartinds, NULL, 
                            1, 0, 5, NULL);
            break;
        case 2:
            rb->set_int("Human starting farms", "", UNIT_INT, 
                            &superdom_settings.humanstartfarms, NULL, 
                            1, 0, 5, NULL);
            break;
        case 3:
            rb->set_int("Human starting factories", "", UNIT_INT,
                            &superdom_settings.humanstartinds, NULL,
                            1, 0, 5, NULL);
            break;
        case 4:
            rb->set_int("Starting cash", "", UNIT_INT, 
                            &superdom_settings.startcash, NULL, 
                            250, 0, 5000, NULL);
            break;
        case 5:
            rb->set_int("Starting food", "", UNIT_INT, 
                            &superdom_settings.startfood, NULL, 
                            250, 0, 5000, NULL);
            break;
        case 6:
            rb->set_int("Moves per turn", "", UNIT_INT, 
                            &superdom_settings.movesperturn, NULL,
                            1, 1, 5, NULL);
            break;
        case MENU_ATTACHED_USB:
            return RET_VAL_USB;
            break;
        case GO_TO_PREVIOUS:
            return RET_VAL_OK;
            break;
        }
    }
    return RET_VAL_OK;
}

static int superdom_help(void) {
    static char* help_text[] = {
        "Super", "domination", "is", "a", "turn", "based", "strategy", "game,",
        "where", "the", "aim", "is", "to", "overpower", "the", "computer",
        "player", "by", "taking", "their", "territory.", "",
        "Each", "year", "you", "are", "allocated", "an", "amount", "of", "cash",
        "and", "food,", "depending", "on", "how", "many", "farms", "and",
        "factories", "you", "control.", "",
        "Use", "this", "cash", "and", "food", "to", "buy", "and", "feed", "your",
        "army.", "Each", "tile", "has", "a", "strength,", "calculated", "by",
        "the", "ownership", "of", "adjacent", "tiles,", "and", "the", "type",
        "and", "number", "of", "troops", "on", "them.",
    };

    if (display_text(ARRAYLEN(help_text), help_text, NULL, NULL, true))
        return RET_VAL_USB;
    return RET_VAL_OK;
}

int start_menu(void) {
    int selection = 0;

    MENUITEM_STRINGLIST(menu, "Super Domination Menu", NULL,
                    "Play Super Domination", "Settings",
                    "Help", "Playback Control", "Quit");

    while(1) {
        switch(rb->do_menu(&menu, &selection, NULL, false)) {
            case 0:
                return RET_VAL_OK; /* start playing */
                break;
            case 1:
                if(settings_menu()==RET_VAL_USB)
                    return RET_VAL_USB;
                break;
            case 2:
                if(superdom_help()==RET_VAL_USB)
                    return RET_VAL_USB;
                break;
            case 3:
                if(playback_control(NULL))
                    return RET_VAL_USB;
                break;
            case 4:
                return RET_VAL_QUIT_ERR;
                break;
        }
    }
    return RET_VAL_QUIT_ERR;
}

int save_game(void) {
    int fd;
    char savepath[MAX_PATH];

    rb->snprintf(savepath, sizeof(savepath), "/Savegame.ssg");
    if(rb->kbd_input(savepath, MAX_PATH)) {
        DEBUGF("Keyboard input failed\n");
        return -1;
    }

    fd = rb->open(savepath, O_WRONLY|O_CREAT);
    DEBUGF("savepath: %s\n", savepath);
    if(fd < 0) {
        DEBUGF("Couldn't create/open file\n");
        return -1;
    }

    rb->write(fd, "SSGv3", 5);
    rb->write(fd, &gamestate, sizeof(gamestate));
    rb->write(fd, &humanres.cash, sizeof(humanres.cash));
    rb->write(fd, &humanres.food, sizeof(humanres.food));
    rb->write(fd, &humanres.bank, sizeof(humanres.bank));
    rb->write(fd, &humanres.planes, sizeof(humanres.planes));
    rb->write(fd, &humanres.tanks, sizeof(humanres.tanks));
    rb->write(fd, &humanres.men, sizeof(humanres.men));
    rb->write(fd, &humanres.nukes, sizeof(humanres.nukes));
    rb->write(fd, &humanres.inds, sizeof(humanres.inds));
    rb->write(fd, &humanres.farms, sizeof(humanres.farms));
    rb->write(fd, &humanres.moves, sizeof(humanres.moves));
    rb->write(fd, &compres.cash, sizeof(compres.cash));
    rb->write(fd, &compres.food, sizeof(compres.food));
    rb->write(fd, &compres.bank, sizeof(compres.bank));
    rb->write(fd, &compres.planes, sizeof(compres.planes));
    rb->write(fd, &compres.tanks, sizeof(compres.tanks));
    rb->write(fd, &compres.men, sizeof(compres.men));
    rb->write(fd, &compres.nukes, sizeof(compres.nukes));
    rb->write(fd, &compres.inds, sizeof(compres.inds));
    rb->write(fd, &compres.farms, sizeof(compres.farms));
    rb->write(fd, &compres.moves, sizeof(compres.moves));
    rb->write(fd, board, sizeof(board));
    rb->write(fd, &superdom_settings.compstartfarms, sizeof(int));
    rb->write(fd, &superdom_settings.compstartinds, sizeof(int));
    rb->write(fd, &superdom_settings.humanstartfarms, sizeof(int));
    rb->write(fd, &superdom_settings.humanstartinds, sizeof(int));
    rb->write(fd, &superdom_settings.startcash, sizeof(int));
    rb->write(fd, &superdom_settings.startfood, sizeof(int));
    rb->write(fd, &superdom_settings.movesperturn, sizeof(int));
    rb->close(fd);
    return 0;
}

int ingame_menu(void) {
    MENUITEM_STRINGLIST(menu, "Super Domination Menu", NULL,
                    "Return to game", "Save Game",
                    "Playback Control", "Quit");

    switch(rb->do_menu(&menu, NULL, NULL, false)) {
        case 0:
            return RET_VAL_OK;
            break;
        case 1:
            if(!save_game())
                rb->splash(HZ, "Game saved");
            else
                rb->splash(HZ, "Error in save");
            break;
        case 2:
            if(playback_control(NULL))
                return RET_VAL_USB;
            break;
        case 3:
            return RET_VAL_QUIT_ERR;
            break;
        case MENU_ATTACHED_USB:
            return RET_VAL_USB;
            break;
        case GO_TO_PREVIOUS:
            return RET_VAL_OK;
            break;
    }
    return RET_VAL_OK;
}

int get_number(char* param, int* value, int max) {
    static const char *button_labels[4][3] = {
        { "1", "2", "3" },
        { "4", "5", "6" },
        { "7", "8", "9" },
        { "CLR", "0", "OK" }
    };
    int i,j,x=0,y=0;
    int height, width;
    int button = 0, ret = RET_VAL_OK;
    bool done = false;
    rb->lcd_clear_display();
    rb->lcd_getstringsize("CLR", &width, &height);
    if(width > NUM_BOX_WIDTH || height > NUM_BOX_HEIGHT)
        rb->lcd_setfont(FONT_SYSFIXED);
    /* Draw a 3x4 grid */
    for(i=0;i<=3;i++) {  /* Vertical lines */
        rb->lcd_vline(NUM_MARGIN_X+(NUM_BOX_WIDTH*i), NUM_MARGIN_Y,
                      NUM_MARGIN_Y+(4*NUM_BOX_HEIGHT));
    }
    for(i=0;i<=4;i++) {  /* Horizontal lines */
        rb->lcd_hline(NUM_MARGIN_X, NUM_MARGIN_X+(3*NUM_BOX_WIDTH),
                      NUM_MARGIN_Y+(NUM_BOX_HEIGHT*i));
    }
    for(i=0;i<4;i++) {
        for(j=0;j<3;j++) {
            rb->lcd_getstringsize(button_labels[i][j], &width, &height);
            rb->lcd_putsxy(
                    NUM_MARGIN_X+(j*NUM_BOX_WIDTH)+NUM_BOX_WIDTH/2-width/2,
                    NUM_MARGIN_Y+(i*NUM_BOX_HEIGHT)+NUM_BOX_HEIGHT/2-height/2,
                    button_labels[i][j]);
        }
    }
    rb->snprintf(buf,sizeof(buf), "%d", *value);
    rb->lcd_putsxy(NUM_MARGIN_X+10, NUM_MARGIN_Y+4*NUM_BOX_HEIGHT+10, buf);
    rb->lcd_getstringsize(param, &width, &height);
    if(width < LCD_WIDTH)
        rb->lcd_putsxy((LCD_WIDTH-width)/2, (NUM_MARGIN_Y-height)/2, param);
    else
        rb->lcd_puts_scroll(0, (NUM_MARGIN_Y/height-1)/2, param);
    rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
    rb->lcd_fillrect(NUM_MARGIN_X+(NUM_BOX_WIDTH*x), 
                    NUM_MARGIN_Y+(NUM_BOX_HEIGHT*y), 
                    NUM_BOX_WIDTH+1, NUM_BOX_HEIGHT+1);
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_update();
    while(!done) {
        button = rb->button_get(true);
        rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
        rb->lcd_fillrect(NUM_MARGIN_X+(NUM_BOX_WIDTH*x),
                        NUM_MARGIN_Y+(NUM_BOX_HEIGHT*y),
                        NUM_BOX_WIDTH+1, NUM_BOX_HEIGHT+1);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        switch(button) {
            case SUPERDOM_OK:
                if(y!=3) {
                    *value *= 10;
                    *value += button_labels[y][x][0] - '0';
                } else if(x==0) {
                    *value /= 10;
                } else if(x==1) {
                    *value *= 10;
                } else if(x==2) {
                    done = true;
                    break;
                }
                if ((unsigned) *value > (unsigned) max)
                    *value = max;
                rb->lcd_set_drawmode(DRMODE_BG|DRMODE_INVERSEVID);
                rb->lcd_fillrect(0, NUM_MARGIN_Y+4*NUM_BOX_HEIGHT+10,
                                LCD_WIDTH, 30);
                rb->lcd_set_drawmode(DRMODE_SOLID);
                rb->snprintf(buf,sizeof(buf), "%d", *value);
                rb->lcd_putsxy(NUM_MARGIN_X+10, 
                                NUM_MARGIN_Y+4*NUM_BOX_HEIGHT+10, buf);
                break;
            case SUPERDOM_CANCEL:
                *value = 0;
                done = true;
                ret = RET_VAL_QUIT_ERR;
                break;
#if CONFIG_KEYPAD != IRIVER_H10_PAD
            case SUPERDOM_LEFT:
                if(x==0) {
#ifdef IPOD_STYLE
                    if(y>0)
                        y--;
                    else
                        y=3;
#endif
                    x=2;
                } else {
                    x--;
                }
                break;
            case SUPERDOM_RIGHT:
                if(x==2) {
#ifdef IPOD_STYLE
                    if(y==3)
                        y=0;
                    else
                        y++;
#endif
                    x=0;
                } else {
                    x++;
                }
                break;
#endif
#ifndef IPOD_STYLE
            case SUPERDOM_UP:
                if(y==0) {
#if CONFIG_KEYPAD == IRIVER_H10_PAD
                    if(x > 0)
                        x--;
                    else
                        x=2;
#endif
                    y=3;
                } else {
                    y--;
                }
                break;
            case SUPERDOM_DOWN:
                if(y==3) {
#if CONFIG_KEYPAD == IRIVER_H10_PAD
                    if(x < 2)
                        x++;
                    else
                        x=0;
#endif
                    y=0;
                } else {
                    y++;
                }
                break;
#endif
            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                {
                    done = true;
                    ret = RET_VAL_USB;
                }
                break;
        }
        rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
        rb->lcd_fillrect(NUM_MARGIN_X+(NUM_BOX_WIDTH*x), 
                        NUM_MARGIN_Y+(NUM_BOX_HEIGHT*y), 
                        NUM_BOX_WIDTH+1, NUM_BOX_HEIGHT+1);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_update();
    }
    rb->lcd_setfont(FONT_UI);
    rb->lcd_stop_scroll();
    if (ret == RET_VAL_QUIT_ERR)
        rb->splash(HZ, "Cancelled");
    return ret;
}

bool tile_has_item(int type, int x, int y) {
    switch(type) {
        case 0:
            return (board[x][y].men > 0);
            break;
        case 1:
            return board[x][y].tank;
            break;
        case 2:
            return board[x][y].plane;
            break;
        case 3:
            return board[x][y].farm;
            break;
        case 4:
            return board[x][y].ind;
            break;
        case 5:
            return board[x][y].nuke;
            break;
    }
    return false;
}

int buy_resources(int colour, int type, int x, int y, int nummen) {
    const char *itemnames[][6] = {
        {
            "them",
            "the tank",
            "the plane",
            "the farm",
            "the industrial plant",
            "the nuke",
        }, {
            "place men",
            "place a tank",
            "place a plane",
            "build a farm",
            "build an industrial plant",
            "place a nuke",
        }, {
            NULL,
            "a tank",
            "a plane",
            "a farm",
            "an industrial plant",
            "a nuke",
        },
    };

    bool human = (colour == COLOUR_LIGHT);
    int price = 0;
    int temp;
    struct resources *res;

    if(human) {
        res = &humanres;
    } else {
        res = &compres;
    }
    switch(type) {
        case 0: /* men */
            price = 1*nummen;
            break;
        case 1: /* tank */
            price = 300;
            break;
        case 2: /* plane */
            price = 600;
            break;
        case 3: /* Farm */
            price = 1150;
            break;
        case 4: /* Factory */
            price = 1300;
            break;
        case 5: /* nuke */
            price = 2000;
            break;
    }
    if(res->cash < price) {
        if(human)
            rb->splash(HZ, "Not enough money!");
        return RET_VAL_QUIT_ERR;
    }
    if(human) {
        rb->splashf(HZ, "Where do you want to place %s?", itemnames[0][type]);
        if((temp = select_square()) != RET_VAL_OK)
            return temp;
        x = cursor.x;
        y = cursor.y;
    }
    if(board[x][y].colour != colour) {
        if(human)
            rb->splashf(HZ, "Can't %s on enemy territory", itemnames[1][type]);
        return RET_VAL_QUIT_ERR;
    }
    if(type != 0 && tile_has_item(type, x, y)) {
        if(human)
            rb->splashf(HZ, "There is already %s there", itemnames[2][type]);
        return RET_VAL_QUIT_ERR;
    }
    switch(type) {
        case 0:
            board[x][y].men += nummen;
            res->men += nummen;
            break;
        case 1:
            board[x][y].tank = true;
            res->tanks++;
            break;
        case 2:
            board[x][y].plane = true;
            res->planes++;
            break;
        case 3:
            board[x][y].farm = true;
            res->farms++;
            break;
        case 4:
            board[x][y].ind = true;
            res->inds++;
            break;
        case 5:
            board[x][y].nuke = true;
            res->nukes++;
            break;
    }
    res->cash -= price;

    draw_board();
    rb->sleep(HZ);

    return RET_VAL_OK;
}

int buy_resources_menu(void) {
    int selection = 0,nummen;

    MENUITEM_STRINGLIST(menu, "Buy Resources", NULL,
                    "Buy men ($1)", "Buy tank ($300)", "Buy plane ($600)",
                    "Buy Farm ($1150)", "Buy Factory ($1300)",
                    "Buy Nuke ($2000)",
                    "Finish buying");

    while(1) {
        switch(rb->do_menu(&menu, &selection, NULL, false)) {
            case 0:
                nummen = 0;
                if(get_number("How many men would you like?", &nummen,
                                humanres.cash) == RET_VAL_USB)
                    return RET_VAL_USB;
                if(!nummen)
                    break;
                /* fall through */
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
                if(buy_resources(COLOUR_LIGHT, selection, 0, 0, nummen)
                                    == RET_VAL_USB)
                    return RET_VAL_USB;
                break;
            case 6:
                return RET_VAL_OK;
                break;
            case MENU_ATTACHED_USB:
                return RET_VAL_USB;
                break;
            case GO_TO_PREVIOUS:
                return RET_VAL_OK;
                break;
        }
    }
    return RET_VAL_OK;
}

int move_unit(int colour, int type, int fromx, int fromy,
                int tox, int toy, int nummen) {
    const char *itemnames[][3] = {
        {
            "troops",
            "the tank",
            "the plane",
        }, {
            "any troops",
            "a tank",
            "a plane",
        }, {
            "the troops",
            "the tank",
            "the plane",
        }
    };
    bool human = (colour == COLOUR_LIGHT);
    int temp;

    if(human) {
        rb->splashf(HZ, "Select where you want to move %s from",
                        itemnames[0][type]);
        if((temp = select_square()) != RET_VAL_OK)
            return temp;
        fromx = cursor.x;
        fromy = cursor.y;
    }
    if(board[fromx][fromy].colour != colour) {
        if(human)
            rb->splash(HZ, "That isn't your territory");
        return RET_VAL_QUIT_ERR;
    }
    if(!tile_has_item(type, fromx, fromy)) {
        if(human)
            rb->splashf(HZ, "You don't have %s there", itemnames[1][type]);
        return RET_VAL_QUIT_ERR;
    }
    if(type == 0) {
        if(human) {
            nummen = board[fromx][fromy].men;
            if((temp = get_number("How many men do you want to move?", &nummen,
                        nummen)) != RET_VAL_OK)
                return temp;
        }
        if(nummen > board[fromx][fromy].men) {
            if(human)
                rb->splash(HZ, "You don't have that many troops.");
            return RET_VAL_QUIT_ERR;
        }
    }
    if(human) {
        rb->splashf(HZ, "Select where you want to move %s to",
                        itemnames[2][type]);
        if((temp = select_square()) != RET_VAL_OK)
            return temp;
        tox = cursor.x;
        toy = cursor.y;
    }
    if((tox == fromx && toy == fromy) ||
       board[tox][toy].colour != colour ||
       (type != 2 && (abs(tox - fromx) > 1 || abs(toy - fromy) > 1))) {
        if(human)
            rb->splash(HZ, "Invalid move");
        return RET_VAL_QUIT_ERR;
    }
    if(type != 0 && tile_has_item(type, tox, toy)) {
        if(human)
            rb->splashf(HZ, "There is already %s there", itemnames[1][type]);
        return RET_VAL_QUIT_ERR;
    }
    switch(type) {
        case 0:
            board[fromx][fromy].men -= nummen;
            board[tox][toy].men += nummen;
            break;
        case 1:
            board[fromx][fromy].tank = false;
            board[tox][toy].tank = true;
            break;
        case 2:
            board[fromx][fromy].plane = false;
            board[tox][toy].plane = true;
            break;
    }
    return RET_VAL_OK;
}

int move_unit_menu(void) {
    int selection = 0;

    MENUITEM_STRINGLIST(menu, "Move unit", NULL,
                    "Move men", "Move tank", "Move plane");
    switch(rb->do_menu(&menu, &selection, NULL, false)) {
        case 0:
        case 1:
        case 2:
            switch(move_unit(COLOUR_LIGHT, selection, 0, 0, 0, 0, 0)) {
                case RET_VAL_OK:
                    humanres.moves--;
                    break;
                case RET_VAL_USB:
                    return RET_VAL_USB;
                    break;
            }
            break;
        case MENU_ATTACHED_USB:
            return RET_VAL_USB;
    }
    return RET_VAL_OK;
}

int launch_nuke(int colour, int nukex, int nukey, int targetx, int targety) {
    bool human = (colour == COLOUR_LIGHT);
    int temp;
    struct resources *res;

    if(board[nukex][nukey].colour != colour) {
        if(human)
            rb->splash(HZ, "That isn't your territory");
        return RET_VAL_QUIT_ERR;
    }
    if(! board[nukex][nukey].nuke) {
        if(human)
            rb->splashf(HZ, "You don't have %s there", "a nuke");
        return RET_VAL_QUIT_ERR;
    }
    if(human) {
        rb->splash(HZ, "Select place to target with nuke");
        if((temp = select_square()) != RET_VAL_OK)
            return temp;
        targetx = cursor.x;
        targety = cursor.y;
    }
    if(human) {
        humanres.nukes--;
    } else {
        compres.nukes--;
    }
    board[nukex][nukey].nuke = false;

    if(board[targetx][targety].colour == COLOUR_LIGHT) {
        res = &humanres;
    } else {
        res = &compres;
    }
    res->men -= board[targetx][targety].men;
    res->tanks -= board[targetx][targety].tank;
    res->planes -= board[targetx][targety].plane;
    res->nukes -= board[targetx][targety].nuke;
    res->farms -= board[targetx][targety].farm;
    res->inds -= board[targetx][targety].ind;
    board[targetx][targety].men = 0;
    board[targetx][targety].tank = false;
    board[targetx][targety].plane = false;
    board[targetx][targety].ind = false;
    board[targetx][targety].nuke = false;
    board[targetx][targety].farm = false;
    /* TODO: Fallout carried by wind */

    return RET_VAL_OK;
}

int movement_menu(void) {
    int selection = 0, temp;

    MENUITEM_STRINGLIST(menu, "Movement", NULL,
                    "Move unit", "Buy additional moves ($100)",
                    "Launch nuclear missile", "Check map",
                    "Finish moving", "Game menu");

    while(1) {
        switch(rb->do_menu(&menu, &selection, NULL, false)) {
            case 0:
                if(humanres.moves) {
                    if(move_unit_menu()==RET_VAL_USB)
                        return RET_VAL_USB;
                } else {
                    rb->splash(HZ, "You have no more moves left." 
                                   " You can buy more for $100 each.");
                }
                break;
            case 1:
                if(humanres.cash > 100) {
                    humanres.moves++;
                    humanres.cash -= 100;
                    rb->snprintf(buf, sizeof(buf), "You now have %d moves", 
                                    humanres.moves);
                    rb->splash(HZ, buf);
                }
                break;
            case 2:
                if(humanres.nukes==0) {
                    rb->splash(HZ, "You do not have any nukes to launch");
                } else {
                    rb->splash(HZ, "Select place to launch nuke from");
                    switch(select_square()) {
                        case RET_VAL_OK:
                            if(launch_nuke(COLOUR_LIGHT, cursor.x, cursor.y,
                                           0, 0) == RET_VAL_USB)
                                return RET_VAL_USB;
                            break;
                        case RET_VAL_USB:
                            return RET_VAL_USB;
                            break;
                    }
                }
                break;
            case 3:
                if(select_square() == RET_VAL_USB)
                    return RET_VAL_USB;
                break;
            case 4:
                return RET_VAL_OK;
                break;
            case 5:
                if((temp = ingame_menu()) != RET_VAL_OK)
                    return temp;
                break;
            case MENU_ATTACHED_USB:
                return RET_VAL_USB;
                break;
        }
    }
    return RET_VAL_OK;
}

static const char* inventory_data(int selected_item, void * data,
                                  char * buffer, size_t buffer_len) {
    (void)data;
    switch(selected_item) {
        case 0:
            rb->snprintf(buffer,buffer_len,"Men: %d", humanres.men);
            break;
        case 1:
            rb->snprintf(buffer,buffer_len,"Tanks: %d", humanres.tanks);
            break;
        case 2:
            rb->snprintf(buffer,buffer_len,"Planes: %d", humanres.planes);
            break;
        case 3:
            rb->snprintf(buffer,buffer_len,"Factories: %d", humanres.inds);
            break;
        case 4:
            rb->snprintf(buffer,buffer_len,"Farms: %d", humanres.farms);
            break;
        case 5:
            rb->snprintf(buffer,buffer_len,"Nukes: %d", humanres.nukes);
            break;
        case 6:
            rb->snprintf(buffer,buffer_len,"Cash: %d", humanres.cash);
            break;
        case 7:
            rb->snprintf(buffer,buffer_len,"Food: %d", humanres.food);
            break;
        case 8:
            rb->snprintf(buffer,buffer_len,"Bank: %d", humanres.bank);
            break;
        default:
            return NULL;
    }
    return buffer;
}

int show_inventory(void) {
    struct simplelist_info info;
    rb->simplelist_info_init(&info, "Inventory", 9, NULL);
    info.hide_selection = true;
    info.get_name = inventory_data;
    if(rb->simplelist_show_list(&info)) {
        return RET_VAL_USB;
    } else {
        return RET_VAL_OK;
    }
}

int production_menu(void) {
    int selection = 0, temp;

    MENUITEM_STRINGLIST(menu, "Production", NULL,
                    "Buy resources", "Show inventory", "Check map",
                    "Invest money", "Withdraw money",
                    "Finish turn", "Game menu");

    while(1) {
        switch(rb->do_menu(&menu, &selection, NULL, false)) {
            case 0:
                if(buy_resources_menu() == RET_VAL_USB)
                    return RET_VAL_USB;
                break;
            case 1:
                if(show_inventory() == RET_VAL_USB)
                    return RET_VAL_USB;
                break;
            case 2:
                if(select_square() == RET_VAL_USB)
                    return RET_VAL_USB;
                break;
            case 3:
                temp = humanres.cash;
                if(get_number("How much do you want to invest?", &temp,
                                humanres.cash) == RET_VAL_USB)
                    return RET_VAL_USB;
                if(temp > humanres.cash) {
                    rb->splash(HZ, "You don't have that much cash to invest");
                } else {
                    humanres.cash -= temp;
                    humanres.bank += temp;
                }
                break;
            case 4:
                temp = humanres.bank;
                if(get_number("How much do you want to withdraw?", &temp,
                                humanres.bank) == RET_VAL_USB)
                    return RET_VAL_USB;
                if(temp > humanres.bank) {
                    rb->splash(HZ, "You don't have that much cash to withdraw");
                } else {
                    humanres.cash += temp;
                    humanres.bank -= temp;
                }
                break;
            case 5:
                return RET_VAL_OK;
                break;
            case 6:
                if((temp = ingame_menu()) != RET_VAL_OK)
                    return temp;
                break;
            case MENU_ATTACHED_USB:
                return RET_VAL_USB;
                break;
        }
    }
    return RET_VAL_OK;
}

void init_resources(void) {
    humanres.cash = superdom_settings.startcash;
    humanres.food = superdom_settings.startfood;
    humanres.tanks = 0;
    humanres.planes = 0;
    humanres.nukes = 0;
    humanres.inds = 0;
    humanres.farms = 0;
    humanres.men = 0;
    humanres.bank = 0;
    humanres.moves = 0;
    compres.cash = superdom_settings.startcash;
    compres.food = superdom_settings.startfood;
    compres.tanks = 0;
    compres.planes = 0;
    compres.nukes = 0;
    compres.inds = 0;
    compres.farms = 0;
    compres.men = 0;
    compres.bank = 0;
    compres.moves = 0;
}

int select_square(void) {
    int button = 0;
    draw_board();
    draw_cursor();
    update_score();
#if LCD_WIDTH >= 220
    rb->lcd_setfont(FONT_SYSFIXED);
    rb->snprintf(buf, sizeof(buf), "Cash: %d", humanres.cash);
    rb->lcd_putsxy(125, LCD_HEIGHT-20, buf);
    rb->snprintf(buf, sizeof(buf), "Food: %d", humanres.food);
    rb->lcd_putsxy(125, LCD_HEIGHT-10, buf);
    rb->lcd_setfont(FONT_UI);
#endif
    rb->lcd_update();
    while(1) {
        button = rb->button_get(true);
        switch(button) {
            case SUPERDOM_CANCEL:
                rb->splash(HZ, "Cancelled");
                return RET_VAL_QUIT_ERR;
                break;
            case SUPERDOM_OK:
                return RET_VAL_OK;
                break;
#if CONFIG_KEYPAD != IRIVER_H10_PAD
            case SUPERDOM_LEFT:
            case (SUPERDOM_LEFT|BUTTON_REPEAT):
                draw_cursor(); /* Deselect the current tile */
                if(cursor.x>1) {
                    cursor.x--;
                } else {
#ifdef IPOD_STYLE
                    if(cursor.y>1)
                        cursor.y--;
                    else
                        cursor.y = 10;
#endif
                    cursor.x = 10;
                }
                update_score();
                draw_cursor();
                break;
            case SUPERDOM_RIGHT:
            case (SUPERDOM_RIGHT|BUTTON_REPEAT):
                draw_cursor(); /* Deselect the current tile */
                if(cursor.x<10) {
                    cursor.x++;
                } else {
#ifdef IPOD_STYLE
                    if(cursor.y<10)
                        cursor.y++;
                    else
                        cursor.y = 1;
#endif
                    cursor.x = 1;
                }
                update_score();
                draw_cursor();
                break;
#endif
#ifndef IPOD_STYLE
            case SUPERDOM_UP:
            case (SUPERDOM_UP|BUTTON_REPEAT):
                draw_cursor(); /* Deselect the current tile */
                if(cursor.y>1) {
                    cursor.y--;
                } else {
#if CONFIG_KEYPAD == IRIVER_H10_PAD
                    if(cursor.x > 1)
                        cursor.x--;
                    else
                        cursor.x = 10;
#endif
                    cursor.y = 10;
                }
                update_score();
                draw_cursor();
                break;
            case SUPERDOM_DOWN:
            case (SUPERDOM_DOWN|BUTTON_REPEAT):
                draw_cursor(); /* Deselect the current tile */
                if(cursor.y<10) {
                    cursor.y++;
                } else {
#if CONFIG_KEYPAD == IRIVER_H10_PAD
                    if(cursor.x < 10)
                        cursor.x++;
                    else
                        cursor.x = 1;
#endif
                    cursor.y = 1;
                }
                update_score();
                draw_cursor();
                break;
#endif
            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                {
                    return RET_VAL_USB;
                }
        }
    }
}

int killmen(int colour) {
    bool human = (colour == COLOUR_LIGHT);
    int menkilled,i,j;
    int percent;
    if(human) {
        percent = (humanres.food*1000)/humanres.men;
        humanres.food = 0;
    } else {
        percent = (compres.food*1000)/compres.men;
        compres.food = 0;
    }
    menkilled = 0;
    for(i=1;i<11;i++) {
        for(j=1;j<11;j++) {
            if(board[i][j].colour == colour) {
                int nummen = ((board[i][j].men * percent)/1000);
                menkilled += board[i][j].men - nummen;
                board[i][j].men = nummen;
            }
        }
    }

    if(human)
        humanres.men -= menkilled;
    else
        compres.men -= menkilled;
    return menkilled;
}

/* return -1 if error, 1 if attack is succeeded, 0 otherwise */
int attack_territory(int colour, int x, int y) {
    bool human = (colour == COLOUR_LIGHT);
    int str_diff;

    if(board[x][y].colour == colour) {
        if(human)
            rb->splash(HZ, "You can't attack your own territory");
        return -1;
    }
    str_diff = calc_strength(COLOUR_DARK, x, y) -
               calc_strength(COLOUR_LIGHT, x, y);
    if(human) {
        str_diff = -str_diff;
    }
    rb->srand(*rb->current_tick);
    if(str_diff > 0 || (str_diff == 0 && rb->rand()%2)) {
        struct resources *offres, *defres;
        if(human) {
            offres = &humanres;
            defres = &compres;
        } else {
            offres = &compres;
            defres = &humanres;
        }
        defres->men -= board[x][y].men;
        defres->tanks -= board[x][y].tank;
        defres->planes -= board[x][y].plane;
        defres->nukes -= board[x][y].nuke;
        defres->farms -= board[x][y].farm;
        defres->inds -= board[x][y].ind;
        offres->farms += board[x][y].farm;
        offres->inds += board[x][y].ind;
        board[x][y].colour = colour;
        board[x][y].men = 0;
        board[x][y].tank = false;
        board[x][y].plane = false;
        board[x][y].nuke = false;
        draw_board();
        if(human)
            rb->sleep(HZ*2);
        else
            rb->sleep(HZ);
        return 1;
    } else {
        if(human)
            rb->splash(HZ, "Your troops were unable to overcome"
                           " the enemy troops");
        else
            rb->splash(HZ*2, "The computer attempted to "
                            "attack, but the invasion was"
                            " pushed back");
        return 0;
    }
    return 0;
}

int war_menu(void) {
    int selection = 0, temp;

    MENUITEM_STRINGLIST(menu, "War!", NULL,
                    "Select territory to attack",
                    "Finish turn", "Game menu");

    while(humanres.moves) {
        switch(rb->do_menu(&menu, &selection, NULL, false)) {
            case 0:
                switch(select_square()) {
                    case RET_VAL_OK:
                        if(attack_territory(COLOUR_LIGHT, cursor.x, cursor.y)
                                >= 0)
                            humanres.moves--;
                        break;
                    case RET_VAL_USB:
                        return RET_VAL_USB;
                        break;
                }
                break;
            case 1:
                return RET_VAL_OK;
                break;
            case 2:
                if((temp = ingame_menu()) != RET_VAL_OK)
                    return temp;
                break;
        }
    }
    return RET_VAL_OK;
}

struct threat {
    int x;
    int y;
    int str_diff;
};

bool place_adjacent(bool tank, int x, int y) {
    int type = (tank? 1: 2);
    if(!buy_resources(COLOUR_DARK, type, x, y, 0)) {
        return true;
    }
    if(!buy_resources(COLOUR_DARK, type, x-1, y, 0)) {
        return true;
    }
    if(!buy_resources(COLOUR_DARK, type, x+1, y, 0)) {
        return true;
    }
    if(!buy_resources(COLOUR_DARK, type, x, y-1, 0)) {
        return true;
    }
    if(!buy_resources(COLOUR_DARK, type, x, y+1, 0)) {
        return true;
    }
    return false;
}

bool has_adjacent(int x, int y) {
    if((board[x][y].colour == COLOUR_LIGHT) && 
       ((board[x-1][y].colour == COLOUR_DARK) ||
       (board[x+1][y].colour == COLOUR_DARK) ||
       (board[x][y+1].colour == COLOUR_DARK) ||
       (board[x][y-1].colour == COLOUR_DARK)))
        return 1;
    else
        return 0;
}

void find_adjacent(int x, int y, int* adj_x, int* adj_y) {
    /* Finds adjacent squares, returning squares without tanks on them
     * in preference to those with them */
    if(board[x-1][y].colour == COLOUR_DARK) {
        *adj_x = x-1;
        *adj_y = y;
        return;
    }
    if(board[x+1][y].colour == COLOUR_DARK) {
        *adj_x = x+1;
        *adj_y = y;
        return;
    }
    if(board[x][y-1].colour == COLOUR_DARK) {
        *adj_x = x;
        *adj_y = y-1;
        return;
    }
    if(board[x][y+1].colour == COLOUR_DARK) {
        *adj_x = x;
        *adj_y = y+1;
        return;
    }
}

void computer_allocate(void) {
    /* Firstly, decide whether to go offensive or defensive.
     * This is primarily decided by the human player posing a threat to either
     * the computer's farms or factories */
    int i, j, k;
    bool offensive = true;
    struct threat threats[4];
    int numthreats = 0;
    int total_str_diff = 0;
    int numterritory = 0;
    int str_diff;
    int men_needed;
    struct threat targets[2];
    int numtargets;
    struct cursor adj;

    compres.cash += compres.bank;
    compres.bank = 0;
    for(i=1;i<11;i++) {
        for(j=1;j<11;j++) {
            if(board[i][j].colour == COLOUR_DARK) {
                numterritory++;
                str_diff = calc_strength(COLOUR_LIGHT,i,j) -
                           calc_strength(COLOUR_DARK,i,j);
                if(str_diff > 0 && (board[i][j].ind || board[i][j].farm)) {
                    if(numthreats < 3) {
                        offensive = false;
                        threats[numthreats].x = i;
                        threats[numthreats].y = j;
                        threats[numthreats].str_diff = str_diff;
                        numthreats++;
                    }
                }
            }
            rb->yield();
        }
    }
    if(offensive) {
        /* The AI is going to go straight for the throat here and attack
         * the player's farms and factories. The amount of cash
         * the AI has to spend will determine how many targets there are */
        if(compres.cash > 1200) {
            /* 1200 is a figure I pulled out of nowhere. Adjust as needed */
            numtargets = 2;
        } else {
            numtargets = 1;
        }
        /* Work out which target(s) to attack. They must have adjacent squares
         * owned by the computer. If none are found just place troops in
         * random places around the map until we run out of money */
        k = 0;
        for(i=1;i<11;i++) {
            for(j=1;j<11;j++) {
                if(has_adjacent(i,j) &&
                   (board[i][j].ind || board[i][j].farm)) {
                    if(k<numtargets) {
                        targets[k].x = i;
                        targets[k].y = j;
                        targets[k].str_diff =
                                calc_strength(COLOUR_LIGHT, i, j) -
                                calc_strength(COLOUR_DARK, i, j);
                        k++;
                    }
                }
                rb->yield();
            }
        }
        if(k == 0) {
            /* No targets found! Randomly pick squares and if they're owned 
             * by the computer then stick a tank on it. */
                rb->srand(*rb->current_tick);
                while(compres.cash >= 300 && compres.tanks < numterritory) {
                    i = rb->rand()%10 + 1;
                    j = rb->rand()%10 + 1;
                    if(board[i][j].colour == COLOUR_DARK) {
                        buy_resources(COLOUR_DARK, 1, i, j, 0);
                    }
                    rb->yield();
                }
        } else {
            for(i=0;i<k;i++) {
                str_diff = targets[i].str_diff;
                while(str_diff + 20 > 0 && compres.cash > 0) {
                    /* While we still need them keep placing men */
                    if(!place_adjacent(true, targets[i].x, targets[i].y)) {
                        find_adjacent(targets[i].x, targets[i].y,
                            &adj.x, &adj.y);
                        men_needed = (str_diff + 20)*1000/133;
                        if(compres.cash < men_needed) {
                            men_needed = compres.cash;
                        }
                        buy_resources(COLOUR_DARK, 0, adj.x, adj.y,
                                            men_needed);
                        break;
                    }
                    str_diff = calc_strength(COLOUR_LIGHT,
                                      targets[i].x, targets[i].y) -
                               calc_strength(COLOUR_DARK,
                                      targets[i].x, targets[i].y);
                }
            }
        }
    } else {
        /* Work out what to place on each square to defend it.
         * Tanks are preferential because they do not require food,
         * but if the budget is tight then we fall back onto troops.
         * Conversely if cash is not an issue and there are already tanks in
         * place planes will be deployed. We would like a margin of at least
         * 20 points to be safe. */

        for(i=0;i<numthreats;i++) {
            total_str_diff += threats[i].str_diff;
        }
        if((total_str_diff+20)*10 > compres.cash) {
            /* Not enough cash to accomodate all threats using tanks alone -
             * use men as a backup */
            for(i=0;i<numthreats;i++) {
                men_needed = ((threats[i].str_diff + 20)*1000)/133;
                if(compres.cash < men_needed) {
                    men_needed = compres.cash;
                }
                buy_resources(COLOUR_DARK, 0, threats[i].x, threats[i].y,
                                    men_needed);
            }
        } else {
            /* Tanks it is */
            /* Enough money to pay their way by planes? */
            bool tank = ((total_str_diff+20)*15 >= compres.cash);
            for(i=0;i<numthreats;i++) {
                str_diff = threats[i].str_diff;
                while(str_diff + 20 > 0) {
                    if(!place_adjacent(tank, threats[i].x, threats[i].y)) {
                        /* No room for any more planes or tanks, revert to
                         * men */
                        find_adjacent(threats[i].x, threats[i].y,
                            &adj.x, &adj.y);
                        men_needed = (str_diff + 20)*1000/133;
                        if(compres.cash < men_needed) {
                            men_needed = compres.cash;
                        }
                        buy_resources(COLOUR_DARK, 0, threats[i].x,
                                        threats[i].y, men_needed);
                        break;
                    }
                    str_diff = calc_strength(COLOUR_LIGHT,
                                      threats[i].x, threats[i].y) -
                               calc_strength(COLOUR_DARK,
                                      threats[i].x, threats[i].y);
                }
            }
        }
    }
    compres.bank += compres.cash;
    compres.cash = 0;
}

int find_adj_target(int x, int y, struct cursor* adj) {
    /* Find a square next to a computer's farm or factory owned by the player
     * that is vulnerable. Return 1 on success, 0 otherwise */
    if(board[x+1][y].colour == COLOUR_LIGHT && 
         calc_strength(COLOUR_LIGHT,x+1,y)<=calc_strength(COLOUR_DARK,x+1,y)) {
        adj->x = x+1;
        adj->y = y;
        return 1;
    }
    if(board[x-1][y].colour == COLOUR_LIGHT && 
         calc_strength(COLOUR_LIGHT,x-1,y)<=calc_strength(COLOUR_DARK,x-1,y)) {
        adj->x = x-1;
        adj->y = y;
        return 1;
    }
    if(board[x][y+1].colour == COLOUR_LIGHT && 
         calc_strength(COLOUR_LIGHT,x,y+1)<=calc_strength(COLOUR_DARK,x,y+1)) {
        adj->x = x;
        adj->y = y+1;
        return 1;
    }
    if(board[x][y-1].colour == COLOUR_LIGHT && 
         calc_strength(COLOUR_LIGHT,x,y-1)<=calc_strength(COLOUR_DARK,x,y-1)) {
        adj->x = x;
        adj->y = y-1;
        return 1;
    }
    return 0;
}

void computer_war(void) {
    /* Work out where to attack - prioritise the defence of buildings */
    int i, j;
    bool found_target = true;
    struct cursor adj;

    while(found_target) {
        found_target = false;
        for(i=1;i<11;i++) {
            for(j=1;j<11;j++) {
                if((board[i][j].colour == COLOUR_DARK) &&
                   (board[i][j].farm || board[i][j].ind) &&
                   find_adj_target(i, j, &adj)) {
                    found_target = true;
                    if(attack_territory(COLOUR_DARK, adj.x, adj.y) >= 0) {
                        compres.moves--;
                        if(!compres.moves)
                            return;
                    }
                }
                rb->yield();
            }
        }
    }
    /* Defence stage done, move on to OFFENCE */
    found_target = true;
    while(found_target) {
        found_target = false;
        for(i=1;i<11;i++) {
            for(j=1;j<11;j++) {
                if(board[i][j].colour == COLOUR_LIGHT &&
                        (board[i][j].ind || board[i][j].farm) &&
                        (calc_strength(COLOUR_DARK, i, j) >=
                         calc_strength(COLOUR_LIGHT, i, j))) {
                    found_target = true;
                    if(attack_territory(COLOUR_DARK, i, j) >= 0) {
                        compres.moves--;
                        if(!compres.moves)
                            return;
                    }
                }
                rb->yield();
            }
        }
    }
    /* Spend leftover moves wherever attacking randomly */
    found_target = true;
    while(found_target) {
        found_target = false;
        for(i=1;i<11;i++) {
            for(j=1;j<11;j++) {
                if(board[i][j].colour == COLOUR_LIGHT && 
                  (calc_strength(COLOUR_DARK, i, j)  >= 
                   calc_strength(COLOUR_LIGHT, i, j))) {
                    found_target = true;
                    if(attack_territory(COLOUR_DARK, i, j) >= 0) {
                        compres.moves--;
                        if(!compres.moves)
                            return;
                    }
                }
                rb->yield();
            }
        }
    }
}

static int load_game(const char* file) {
    int fd;

    fd = rb->open(file, O_RDONLY);
    if(fd < 0) {
        DEBUGF("Couldn't open savegame\n");
        return -1;
    }
    rb->read(fd, buf, 5);
    if(rb->strcmp(buf, "SSGv3")) {
        rb->splash(HZ, "Invalid/incompatible savegame");
        return -1;
    }
    rb->read(fd, &gamestate, sizeof(gamestate));
    rb->read(fd, &humanres.cash, sizeof(humanres.cash));
    rb->read(fd, &humanres.food, sizeof(humanres.food));
    rb->read(fd, &humanres.bank, sizeof(humanres.bank));
    rb->read(fd, &humanres.planes, sizeof(humanres.planes));
    rb->read(fd, &humanres.tanks, sizeof(humanres.tanks));
    rb->read(fd, &humanres.men, sizeof(humanres.men));
    rb->read(fd, &humanres.nukes, sizeof(humanres.nukes));
    rb->read(fd, &humanres.inds, sizeof(humanres.inds));
    rb->read(fd, &humanres.farms, sizeof(humanres.farms));
    rb->read(fd, &humanres.moves, sizeof(humanres.moves));
    rb->read(fd, &compres.cash, sizeof(humanres.cash));
    rb->read(fd, &compres.food, sizeof(humanres.food));
    rb->read(fd, &compres.bank, sizeof(humanres.bank));
    rb->read(fd, &compres.planes, sizeof(humanres.planes));
    rb->read(fd, &compres.tanks, sizeof(humanres.tanks));
    rb->read(fd, &compres.men, sizeof(humanres.men));
    rb->read(fd, &compres.nukes, sizeof(humanres.nukes));
    rb->read(fd, &compres.inds, sizeof(humanres.inds));
    rb->read(fd, &compres.farms, sizeof(humanres.farms));
    rb->read(fd, &compres.moves, sizeof(humanres.moves));
    rb->read(fd, board, sizeof(board));
    rb->read(fd, &superdom_settings.compstartfarms, sizeof(int));
    rb->read(fd, &superdom_settings.compstartinds, sizeof(int));
    rb->read(fd, &superdom_settings.humanstartfarms, sizeof(int));
    rb->read(fd, &superdom_settings.humanstartinds, sizeof(int));
    rb->read(fd, &superdom_settings.startcash, sizeof(int));
    rb->read(fd, &superdom_settings.startfood, sizeof(int));
    rb->read(fd, &superdom_settings.movesperturn, sizeof(int));
    rb->close(fd);
    return 0;
}

void default_settings(void) {
    superdom_settings.compstartfarms = 1;
    superdom_settings.compstartinds = 1;
    superdom_settings.humanstartfarms = 2;
    superdom_settings.humanstartinds = 2;
    superdom_settings.startcash = 0;
    superdom_settings.startfood = 0;
    superdom_settings.movesperturn = 2;
}

int average_strength(int colour) {
    /* This function calculates the average strength of the given player,
     * used to determine when the computer wins or loses. */
    int i,j;
    int totalpower = 0;
    for(i=1;i<11;i++) {
        for(j=1;j<11;j++) {
            if(board[i][j].colour != -1) {
                totalpower += calc_strength(colour, i, j);
            }
        }
    }
    return totalpower/100;
}

enum plugin_status plugin_start(const void* parameter)
{
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_set_background(LCD_WHITE);
#endif

    cursor.x = 1;
    cursor.y = 1;
    default_settings();
    if(parameter) {
        if(load_game(parameter) != 0) {
            DEBUGF("Loading failed, generating new game\n");
        } else {
            switch(gamestate) {
                case GS_PROD:
                    goto startprod;
                    break;
                case GS_MOVE:
                    goto startmove;
                    break;
                case GS_WAR:
                    goto startwar;
                    break;
                default:
                    goto startyear;
                    break;
            }
        }
    }

    switch(start_menu()) {
        case RET_VAL_OK: /* start playing */
            break;
        case RET_VAL_QUIT_ERR: /* quit */
            return PLUGIN_OK;
            break;
        case RET_VAL_USB:
            return PLUGIN_USB_CONNECTED;
            break;
    }

    init_resources();
    init_board();
    gen_resources();
startyear:
    while(1) {
        int avg_str_diff = (average_strength(COLOUR_LIGHT) -
                            average_strength(COLOUR_DARK));
        if(avg_str_diff > 15) {
            rb->splash(HZ*4, "The computer has surrendered. You win.");
            return PLUGIN_OK;
        }
        if(-avg_str_diff > 15) {
            rb->splash(HZ*4, "Your army have suffered terrible morale from"
                             " the bleak prospects of winning. You lose.");
            return PLUGIN_OK;
        }

        /* production */
startprod:
        gamestate = GS_PROD;
        switch(production_menu()) {
            case RET_VAL_USB:
                return PLUGIN_USB_CONNECTED;
                break;
            case RET_VAL_QUIT_ERR:
                return PLUGIN_OK;
                break;
        }
        computer_allocate();

        /* movement */
        humanres.moves = superdom_settings.movesperturn;
startmove:
        gamestate = GS_MOVE;
        switch(movement_menu()) {
            case RET_VAL_USB:
                return PLUGIN_USB_CONNECTED;
                break;
            case RET_VAL_QUIT_ERR:
                return PLUGIN_OK;
                break;
        }
        /* feed men */
        if(humanres.men) {
            if(humanres.food > humanres.men) {
                rb->snprintf(buf, sizeof(buf), "Your men ate %d units of food",
                                humanres.men);
                humanres.food -= humanres.men;
            } else {
                rb->snprintf(buf, sizeof(buf), "There was not enough food"
                   " to feed all your men, %d men have died of starvation",
                                killmen(COLOUR_LIGHT));
            }
            rb->splash(HZ*2, buf);
        }
        if(compres.men) {
            if(compres.food > compres.men) {
                compres.food -= compres.men;
            } else {
                rb->snprintf(buf, sizeof(buf), "The computer does not have"
                 " enough food to feed its men. %d have died of starvation",
                                killmen(COLOUR_DARK));
                rb->splash(HZ, buf);
            }
        }
        /* war */
        humanres.moves = superdom_settings.movesperturn;
startwar:
        gamestate = GS_WAR;
        switch(war_menu()) {
            case RET_VAL_USB:
                return PLUGIN_USB_CONNECTED;
                break;
            case RET_VAL_QUIT_ERR:
                return PLUGIN_OK;
                break;
        }
        compres.moves = superdom_settings.movesperturn;
        computer_war();
        gen_resources();
    }
    return PLUGIN_OK;
}
