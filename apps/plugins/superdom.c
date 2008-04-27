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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
PLUGIN_HEADER
static struct plugin_api* rb; 

extern const fb_data superdom_boarditems[];
char buf[255];

#define COLOUR_DARK 0
#define COLOUR_LIGHT 1

#define MARGIN 5

#if LCD_WIDTH > LCD_HEIGHT
#define BOX_WIDTH ((LCD_WIDTH-(MARGIN*2))/10)
#define BOX_HEIGHT ((BOX_WIDTH*2)/3)

#else
#define BOX_HEIGHT ((LCD_HEIGHT-(MARGIN*2)-15)/10)
#define BOX_WIDTH ((BOX_HEIGHT*2)/3)

#endif

#if LCD_WIDTH == 220 && LCD_HEIGHT == 176
#define NUM_BOX_HEIGHT 25
#define NUM_BOX_WIDTH 30
#define STRIDE 14
#define ICON_HEIGHT 7
#define ICON_WIDTH 7

#elif (LCD_WIDTH == 160 && LCD_HEIGHT == 128) || \
    (LCD_WIDTH == 176 && LCD_HEIGHT == 132) || \
    (LCD_WIDTH == 176 && LCD_HEIGHT == 220)
#define NUM_BOX_HEIGHT 20
#define NUM_BOX_WIDTH 24
#define STRIDE 8
#define ICON_HEIGHT 4
#define ICON_WIDTH 4

#elif (LCD_WIDTH == 320 && LCD_HEIGHT == 240)
#define NUM_BOX_HEIGHT 25
#define NUM_BOX_WIDTH 30
#define STRIDE 20
#define ICON_HEIGHT 10
#define ICON_WIDTH 10

#elif (LCD_WIDTH == 240 && LCD_HEIGHT == 320)
#define NUM_BOX_HEIGHT 25
#define NUM_BOX_WIDTH 30
#define STRIDE 18
#define ICON_HEIGHT 9
#define ICON_WIDTH 9

#endif

#define NUM_MARGIN_X (LCD_WIDTH-3*NUM_BOX_WIDTH)/2
#define NUM_MARGIN_Y (LCD_HEIGHT-4*NUM_BOX_HEIGHT)/2

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

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define SUPERDOM_OK BUTTON_SELECT
#define SUPERDOM_UP BUTTON_UP
#define SUPERDOM_DOWN BUTTON_DOWN
#define SUPERDOM_LEFT BUTTON_LEFT
#define SUPERDOM_RIGHT BUTTON_RIGHT
#define SUPERDOM_CANCEL BUTTON_BACK

#elif CONFIG_KEYPAD == COWOND2_PAD
#define SUPERDOM_CANCEL BUTTON_POWER

#endif

#ifdef HAVE_TOUCHPAD
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

#define SUPERDOM_QUIT 23

void gen_interest(void);
int production_menu(void);
void init_resources(void);
int select_square(void);
void update_score(void);
void gen_resources(void);
void draw_cursor(void);
int calc_strength(bool colour, int x, int y);
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

struct cursor{
    int x;
    int y;
} cursor;

struct tile board[12][12];

void init_board(void) {
    rb->srand(*rb->current_tick);
    int i,j;
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
            break;
        }
    }
    while(compres.inds < superdom_settings.compstartinds) {
        i = rb->rand()%10 + 1;
        j = rb->rand()%10 + 1;
        if((board[i][j].colour == COLOUR_DARK) && (board[i][j].ind == false)) {
            board[i][j].ind = true;
            compres.inds++;
            break;
        }
    }
    while(humanres.farms<superdom_settings.humanstartfarms) {
        i = rb->rand()%10 + 1;
        j = rb->rand()%10 + 1;
        if((board[i][j].colour == COLOUR_LIGHT)&&(board[i][j].farm == false)) {
            board[i][j].farm = true;
            humanres.farms++;
        }
    }
    while(humanres.inds<superdom_settings.humanstartfarms) {
        i = rb->rand()%10 + 1;
        j = rb->rand()%10 + 1;
        if((board[i][j].colour == COLOUR_LIGHT) && (board[i][j].ind == false)) {
            board[i][j].ind = true;
            humanres.inds++;
        }
    }
}

void draw_board(void) {
    rb->lcd_clear_display();
    int i,j;
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
#if (LCD_DEPTH == 16)
                rb->lcd_bitmap_transparent_part(superdom_boarditems, 
#else
                rb->lcd_mono_bitmap_part(superdom_boarditems,
#endif
                                board[i][j].colour?ICON_WIDTH:0, 0, STRIDE, 
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
#if (LCD_DEPTH == 16)
                rb->lcd_bitmap_transparent_part(superdom_boarditems, 
#else
                rb->lcd_mono_bitmap_part(superdom_boarditems,
#endif
                                board[i][j].colour?ICON_WIDTH:0, ICON_HEIGHT, 
                                STRIDE, MARGIN+(BOX_WIDTH*(i-1))+1, 
                                MARGIN+(BOX_HEIGHT*(j-1))+1, 
                                ICON_WIDTH, ICON_HEIGHT);
            }
            if(board[i][j].tank) {
#if (LCD_DEPTH == 16)
                rb->lcd_bitmap_transparent_part(superdom_boarditems, 
#else
                rb->lcd_mono_bitmap_part(superdom_boarditems,
#endif
                                board[i][j].colour?ICON_WIDTH:0, ICON_HEIGHT*2,
                                STRIDE, MARGIN+(BOX_WIDTH*(i-1))+ICON_WIDTH+1, 
                                MARGIN+(BOX_HEIGHT*(j-1))+ICON_HEIGHT+1, 
                                ICON_WIDTH, ICON_HEIGHT);
            }
            if(board[i][j].men) {
#if (LCD_DEPTH == 16)
                rb->lcd_bitmap_transparent_part(superdom_boarditems, 
#else
                rb->lcd_mono_bitmap_part(superdom_boarditems,
#endif
                                board[i][j].colour?ICON_WIDTH:0, ICON_HEIGHT*3,
#if LCD_WIDTH > LCD_HEIGHT
                                STRIDE, MARGIN+(BOX_WIDTH*(i-1))+ICON_WIDTH+1,
                                MARGIN+(BOX_HEIGHT*(j-1))+1, 
#else
                                STRIDE, MARGIN+(BOX_WIDTH*(i-1))+1,
                                MARGIN+(BOX_HEIGHT*(j-1))+1+ICON_HEIGHT,
#endif
                                ICON_WIDTH, ICON_HEIGHT);
            }
            if(board[i][j].plane) {
#if (LCD_DEPTH == 16)
                rb->lcd_bitmap_transparent_part(superdom_boarditems, 
#else
                rb->lcd_mono_bitmap_part(superdom_boarditems,
#endif
                                board[i][j].colour?ICON_WIDTH:0, ICON_HEIGHT*4,
#if LCD_WIDTH > LCD_HEIGHT
                                STRIDE,MARGIN+(BOX_WIDTH*(i-1))+ICON_WIDTH*2+1,
                                MARGIN+(BOX_HEIGHT*(j-1))+ICON_HEIGHT+1,
#else
                                STRIDE,MARGIN+(BOX_WIDTH*(i-1))+ICON_WIDTH+1,
                                MARGIN+(BOX_HEIGHT*(j-1))+ICON_HEIGHT*2+1,
#endif
                                ICON_WIDTH, ICON_HEIGHT);
            }
            if(board[i][j].nuke) {
#if (LCD_DEPTH == 16)
                rb->lcd_bitmap_transparent_part(superdom_boarditems, 
#else
                rb->lcd_mono_bitmap_part(superdom_boarditems,
#endif
                                board[i][j].colour?ICON_WIDTH:0, ICON_HEIGHT*5,
#if LCD_WIDTH > LCD_HEIGHT
                                STRIDE,MARGIN+(BOX_WIDTH*(i-1))+ICON_WIDTH*2+1,
                                MARGIN+(BOX_HEIGHT*(j-1))+1, 
#else
                                STRIDE,MARGIN+(BOX_WIDTH*(i-1))+1,
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

int calc_strength(bool colour, int x, int y) {
    int a, b, score=0;
    for (a = -1; a < 2; a++){
   for (b = -1; b < 2; b++){
       if (b == 0){
           if(board[x + a][y].colour == colour)
           score+=10;
       if(((board[x + a][y].colour == colour) && board[x + a][y].tank) || ((board[x + a][y].colour == colour) && board[x + a][y].farm))
               score+=30;
       if(((board[x + a][y].colour == colour) && board[x + a][y].plane) || ((board[x + a][y].colour == colour) && board[x + a][y].ind))
               score+=40;
       if((board[x + a][y].colour == colour) && board[x + a][y].nuke)
               score+=20;
       if((board[x + a][y].colour == colour) && board[x + a][y].men)
               score+=(board[x + a][y].men*133/1000);
       } else if (a == 0){
                if(board[x][y + b].colour == colour)
           score+=10;
       if(((board[x][y + b].colour == colour) && board[x][y + b].tank) || ((board[x][y + b].colour == colour) && board[x][y + b].farm))
               score+=30;
       if(((board[x][y + b].colour == colour) && board[x][y + b].plane) || ((board[x][y + b].colour == colour) && board[x][y + b].ind))
               score+=40;
       if((board[x][y + b].colour == colour) && board[x][y + b].nuke)
               score+=20;
       if((board[x][y + b].colour == colour) && board[x][y + b].men)
               score+=(board[x][y + b].men*133/1000);
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
}

void draw_cursor(void) {
    rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
    rb->lcd_fillrect(MARGIN+((cursor.x-1)*BOX_WIDTH), 
                  MARGIN+((cursor.y-1)*BOX_HEIGHT), BOX_WIDTH+1, BOX_HEIGHT+1);
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_update();
}

void gen_resources(void) {
    gen_interest();
    int inccash = 0;
    int incfood = 0;
    int i;
    rb->srand(*rb->current_tick);
    /* Generate Human's resources */
        for(i=0;i<humanres.inds;i++) {
            inccash += (300+rb->rand()%200);
        }
        for(i=0;i<humanres.farms;i++) {
            incfood += (200+rb->rand()%200);
        }
        if(inccash/humanres.inds > 450) {
            if(incfood/humanres.farms > 350) {
                rb->splash(HZ*2, "Patriotism sweeps the land, all production" 
                                " is up this year!");
            } else {
                rb->splash(HZ*2, "Factories working at maximum efficiency," 
                                " cash production up this year!");
            }
        } else if((inccash/humanres.inds>350)&&(inccash/humanres.inds<=450)) {
            if(incfood/humanres.farms > 350) {
                rb->splash(HZ*2, "Record crop harvest this year!");
            } else if((incfood/humanres.farms > 250) &&
                            (incfood/humanres.farms <= 350)) {
                rb->splash(HZ*2, "Production continues as normal");
            } else {
                rb->splash(HZ*2, "Spoilage of crops leads to reduced farm" 
                                " output this  year");
            }
        } else {
            if(incfood/humanres.farms > 350) {
                rb->splash(HZ*2, "Record crop harvest this year!");
            } else if((incfood/humanres.farms > 250) &&
                            (incfood/humanres.farms <= 350)) {
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
    rb->lcd_set_drawmode(DRMODE_BG|DRMODE_INVERSEVID);
    rb->lcd_fillrect(5,LCD_HEIGHT-20,105,20);
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->snprintf(buf, sizeof(buf), "Your power: %d.%d", 
                    calc_strength(COLOUR_LIGHT, cursor.x, cursor.y)/10, 
                    calc_strength(COLOUR_LIGHT, cursor.x, cursor.y)%10);
    rb->lcd_putsxy(5,LCD_HEIGHT-20, buf);
    rb->snprintf(buf, sizeof(buf), "Comp power: %d.%d", 
                    calc_strength(COLOUR_DARK, cursor.x, cursor.y)/10,
                    calc_strength(COLOUR_DARK, cursor.x, cursor.y)%10);
    rb->lcd_putsxy(5,LCD_HEIGHT-10, buf);
}

int settings_menu_function(void) {
    int selection = 0;

    MENUITEM_STRINGLIST(settings_menu,"Super Domination Settings",NULL,
                    "Computer starting farms","Computer starting factories",
                    "Human starting farms","Human starting factories",
                    "Starting cash","Starting food","Moves per turn");
settings_menu:
    selection=rb->do_menu(&settings_menu,&selection, NULL, false);
    switch(selection) {
        case 0:
            rb->set_int("Computer starting farms", "", UNIT_INT, 
                            &superdom_settings.compstartfarms, NULL, 
                            1, 0, 5, NULL);
            goto settings_menu;
            break;
        case 1:
            rb->set_int("Computer starting factories", "", UNIT_INT, 
                            &superdom_settings.compstartinds, NULL, 
                            1, 0, 5, NULL);
            goto settings_menu;
            break;
        case 2:
            rb->set_int("Human starting farms", "", UNIT_INT, 
                            &superdom_settings.humanstartfarms, NULL, 
                            1, 0, 5, NULL);
            goto settings_menu;
            break;
        case 3:
            superdom_settings.humanstartinds = 
                    rb->set_int("Human starting factories", "", UNIT_INT, 
                            &superdom_settings.humanstartinds, NULL, 
                            1, 0, 5, NULL);
            goto settings_menu;
            break;
        case 4:
            rb->set_int("Starting cash", "", UNIT_INT, 
                            &superdom_settings.startcash, NULL, 
                            250, 0, 5000, NULL);
            goto settings_menu;
            break;
        case 5:
            rb->set_int("Starting food", "", UNIT_INT, 
                            &superdom_settings.startfood, NULL, 
                            250, 0, 5000, NULL);
            goto settings_menu;
            break;
        case 6:
            rb->set_int("Moves per turn", "", UNIT_INT, 
                            &superdom_settings.movesperturn, NULL,
                            1, 1, 5, NULL);
            goto settings_menu;
            break;
        case MENU_ATTACHED_USB:
            return PLUGIN_USB_CONNECTED;
            break;
    }
    return 0;
}

static int do_help(void) {
    int button;
    int y = MARGIN, space_w, width, height;
    unsigned short x = MARGIN, i = 0;
#define WORDS (sizeof instructions / sizeof (char*))
    static char* instructions[] = {
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
    rb->lcd_clear_display();
    rb->lcd_getstringsize(" ", &space_w, &height);
    for (i = 0; i < WORDS; i++) {
        rb->lcd_getstringsize(instructions[i], &width, NULL);
        /* Skip to next line if the current one can't fit the word */
        if (x + width > LCD_WIDTH - MARGIN) {
            x = MARGIN;
            y += height;
        }
        /* .. or if the word is the empty string */
        if (rb->strcmp(instructions[i], "") == 0) {
            x = MARGIN;
            y += height;
            continue;
        }
        /* We filled the screen */
        if (y + height > LCD_HEIGHT - MARGIN) {
            y = MARGIN;
            rb->lcd_update();
            do {
                button = rb->button_get(true);
                if (button == SYS_USB_CONNECTED) {
                    return PLUGIN_USB_CONNECTED;
                }
            } while( ( button == BUTTON_NONE )
                    || ( button & (BUTTON_REL|BUTTON_REPEAT) ) );


            rb->lcd_clear_display();
        }
        rb->lcd_putsxy(x, y, instructions[i]);
        x += width + space_w;
    }
    rb->lcd_update();
    do {
        button = rb->button_get(true);
        if (button == SYS_USB_CONNECTED) {
            return PLUGIN_USB_CONNECTED;
        }
    } while( ( button == BUTTON_NONE )
            || ( button & (BUTTON_REL|BUTTON_REPEAT) ) );

    return 0;
}

int menu(void) {
    int selection = 0;
     
    MENUITEM_STRINGLIST(main_menu,"Super Domination Menu",NULL,
                    "Play Super Domination","Settings","Help","Quit");

    while(1) {
        selection=rb->do_menu(&main_menu,&selection, NULL, false);
        switch(selection) {
            case 0:
                return 0; /* start playing */
                break;
            case 1:
                if(settings_menu_function()==PLUGIN_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
            case 2:
                if(do_help()==PLUGIN_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
            default:
                return 2; /* quit program */
                break;
        }
    }

    return 3;
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
    
    rb->write(fd, "SSGv2", 5);
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
    rb->write(fd, &superdom_settings.humanstartfarms, sizeof(int));
    rb->write(fd, &superdom_settings.startcash, sizeof(int));
    rb->write(fd, &superdom_settings.startfood, sizeof(int));
    rb->write(fd, &superdom_settings.movesperturn, sizeof(int));
    rb->close(fd);
    return 0;
}

int ingame_menu(void) {
    int selection = 0;
     
    MENUITEM_STRINGLIST(ingame_menu,"Super Domination Menu",NULL,
                    "Return to game","Save Game", "Quit");

    selection=rb->do_menu(&ingame_menu,&selection, NULL, false);
    switch(selection) {
        case 0:
            return 0;
            break;
        case 1:
            if(!save_game())
                rb->splash(HZ, "Game saved");
            else
                rb->splash(HZ, "Error in save");
            break;
        case 2:
            return SUPERDOM_QUIT;
            break;
        case MENU_ATTACHED_USB:
            return PLUGIN_USB_CONNECTED;
            break;
    }
    return 0;
}

int get_number(char* param, int* value) {
    //int numbers[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    int numbers[3][3];
    numbers[0][0] = 1;
    numbers[0][1] = 2;
    numbers[0][2] = 3;
    numbers[1][0] = 4;
    numbers[1][1] = 5;
    numbers[1][2] = 6;
    numbers[2][0] = 7;
    numbers[2][1] = 8;
    numbers[2][2] = 9;
    rb->lcd_clear_display();
    /* Draw a 3x4 grid */
    int i,j,x=0,y=0;
    for(i=0;i<=3;i++) {  /* Vertical lines */
        rb->lcd_vline(NUM_MARGIN_X+(NUM_BOX_WIDTH*i), NUM_MARGIN_Y,
                      NUM_MARGIN_Y+(4*NUM_BOX_HEIGHT));
    }
    for(i=0;i<=4;i++) {  /* Horizontal lines */
        rb->lcd_hline(NUM_MARGIN_X, NUM_MARGIN_X+(3*NUM_BOX_WIDTH),
                      NUM_MARGIN_Y+(NUM_BOX_HEIGHT*i));
    }
    int temp = 1;
    for(i=0;i<3;i++) {
        for(j=0;j<3;j++) {
            rb->snprintf(buf, sizeof(buf), "%d", temp);
            rb->lcd_putsxy(NUM_MARGIN_X+(j*NUM_BOX_WIDTH)+10, 
                            NUM_MARGIN_Y+(i*NUM_BOX_HEIGHT)+8, buf);
            temp++;
        }
    }
    rb->lcd_putsxy(NUM_MARGIN_X+5, NUM_MARGIN_Y+(3*NUM_BOX_HEIGHT)+8, "CLR");
    rb->lcd_putsxy(NUM_MARGIN_X+NUM_BOX_WIDTH+10, 
                    NUM_MARGIN_Y+(3*NUM_BOX_HEIGHT)+8, "0");
    rb->lcd_putsxy(NUM_MARGIN_X+2*NUM_BOX_WIDTH+8, 
                    NUM_MARGIN_Y+(3*NUM_BOX_HEIGHT)+8, "OK");
    rb->snprintf(buf,sizeof(buf), "%d", *value);
    rb->lcd_putsxy(NUM_MARGIN_X+10, NUM_MARGIN_Y+4*NUM_BOX_HEIGHT+10, buf);
    int height, width;
    rb->lcd_getstringsize(param, &width, &height);
    rb->lcd_putsxy((LCD_WIDTH-width)/2, (NUM_MARGIN_Y-height)/2, param);
    rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
    rb->lcd_fillrect(NUM_MARGIN_X+(NUM_BOX_WIDTH*x), 
                    NUM_MARGIN_Y+(NUM_BOX_HEIGHT*y), NUM_BOX_WIDTH+1, 
                    NUM_BOX_HEIGHT+1);
    rb->lcd_set_drawmode(DRMODE_SOLID);
    int button = 0;
    rb->lcd_update();
    while(1) {
        button = rb->button_get(true);
        switch(button) {
            case SUPERDOM_OK:
                *value *= 10;
                if(y!=3) {
                    *value += numbers[y][x];
                } else if(y==3 && x==0) {
                    *value /= 100;
                } else if(y==3 && x==2) {
                    *value /= 10;
                    return 0;
                }
                rb->lcd_set_drawmode(DRMODE_BG|DRMODE_INVERSEVID);
                rb->lcd_fillrect(0, NUM_MARGIN_Y+4*NUM_BOX_HEIGHT+10,
                                LCD_WIDTH, 30);
                rb->lcd_set_drawmode(DRMODE_SOLID);
                rb->snprintf(buf,sizeof(buf), "%d", *value);
                rb->lcd_putsxy(NUM_MARGIN_X+10, 
                                NUM_MARGIN_Y+4*NUM_BOX_HEIGHT+10, buf);
                break;
            case SUPERDOM_CANCEL:
                return 0;
                break;
#if CONFIG_KEYPAD != IRIVER_H10_PAD
            case SUPERDOM_LEFT:
                rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
                rb->lcd_fillrect(NUM_MARGIN_X+(NUM_BOX_WIDTH*x), 
                                NUM_MARGIN_Y+(NUM_BOX_HEIGHT*y), 
                                NUM_BOX_WIDTH+1, NUM_BOX_HEIGHT+1);
                rb->lcd_set_drawmode(DRMODE_SOLID);
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
                rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
                rb->lcd_fillrect(NUM_MARGIN_X+(NUM_BOX_WIDTH*x), 
                                NUM_MARGIN_Y+(NUM_BOX_HEIGHT*y), 
                                NUM_BOX_WIDTH+1, NUM_BOX_HEIGHT+1);
                rb->lcd_set_drawmode(DRMODE_SOLID);
                break;
            case SUPERDOM_RIGHT:
                rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
                rb->lcd_fillrect(NUM_MARGIN_X+(NUM_BOX_WIDTH*x), 
                                NUM_MARGIN_Y+(NUM_BOX_HEIGHT*y), 
                                NUM_BOX_WIDTH+1, NUM_BOX_HEIGHT+1);
                rb->lcd_set_drawmode(DRMODE_SOLID);
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
                rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
                rb->lcd_fillrect(NUM_MARGIN_X+(NUM_BOX_WIDTH*x), 
                                NUM_MARGIN_Y+(NUM_BOX_HEIGHT*y), 
                                NUM_BOX_WIDTH+1, NUM_BOX_HEIGHT+1);
                rb->lcd_set_drawmode(DRMODE_SOLID);
                break;
#endif
#ifndef IPOD_STYLE
            case SUPERDOM_UP:
                rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
                rb->lcd_fillrect(NUM_MARGIN_X+(NUM_BOX_WIDTH*x), 
                                NUM_MARGIN_Y+(NUM_BOX_HEIGHT*y), 
                                NUM_BOX_WIDTH+1, NUM_BOX_HEIGHT+1);
                rb->lcd_set_drawmode(DRMODE_SOLID);
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
                rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
                rb->lcd_fillrect(NUM_MARGIN_X+(NUM_BOX_WIDTH*x), 
                                NUM_MARGIN_Y+(NUM_BOX_HEIGHT*y), 
                                NUM_BOX_WIDTH+1, NUM_BOX_HEIGHT+1);
                rb->lcd_set_drawmode(DRMODE_SOLID);
                break;
            case SUPERDOM_DOWN:
                rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
                rb->lcd_fillrect(NUM_MARGIN_X+(NUM_BOX_WIDTH*x), 
                                NUM_MARGIN_Y+(NUM_BOX_HEIGHT*y), 
                                NUM_BOX_WIDTH+1, NUM_BOX_HEIGHT+1);
                rb->lcd_set_drawmode(DRMODE_SOLID);
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
                rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
                rb->lcd_fillrect(NUM_MARGIN_X+(NUM_BOX_WIDTH*x), 
                                NUM_MARGIN_Y+(NUM_BOX_HEIGHT*y), 
                                NUM_BOX_WIDTH+1, NUM_BOX_HEIGHT+1);
                rb->lcd_set_drawmode(DRMODE_SOLID);
                break;
#endif
            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                {
                    return PLUGIN_USB_CONNECTED;
                }
                break;
        }
        rb->lcd_update();
    }
    return 0;
}

int buy_resources_menu(void) {
    int selection,tempmenu,nummen;

    MENUITEM_STRINGLIST(res_menu, "Buy Resources", NULL, "Buy men ($1)", 
                    "Buy tank ($300)", "Buy plane ($600)", "Buy Farm ($1150)", 
                    "Buy Factory ($1300)", "Buy Nuke ($2000)", 
                    "Finish buying", "Game menu");

resources_menu:
    selection=rb->do_menu(&res_menu,&selection, NULL, false);
    switch(selection) {
        case 0:
            nummen = 0;
            if(get_number("How many men would you like?", &nummen)
                            == PLUGIN_USB_CONNECTED)
                return PLUGIN_USB_CONNECTED;
            if(humanres.cash>=nummen) {
                rb->splash(HZ, "Where do you want to place them?");
                tempmenu = select_square();
                switch(tempmenu) {
                    case 0:
                        rb->splash(HZ, "Cancelled");
                        break;
                    case 2:
                        if(board[cursor.x][cursor.y].colour == COLOUR_LIGHT) {
                            humanres.men += nummen;
                            board[cursor.x][cursor.y].men += nummen;
                            humanres.cash -= nummen;
                        } else {
                            rb->splash(HZ,"Can't place men on enemy territory");
                        }
                        break;
                    case PLUGIN_USB_CONNECTED:
                        return PLUGIN_USB_CONNECTED;
                        break;
                }
            } else {
                rb->splash(HZ, "Not enough money!");
            }
            goto resources_menu;
            break;
        case 1:
            if(humanres.cash>=300) {
                rb->splash(HZ, "Where do you want to place the tank?");
                tempmenu = select_square();
                switch(tempmenu) {
                    case 0:
                        rb->splash(HZ, "Cancelled");
                        goto resources_menu;
                        break;
                    case PLUGIN_USB_CONNECTED:
                        return PLUGIN_USB_CONNECTED;
                        break;
                }
                if(board[cursor.x][cursor.y].colour == COLOUR_LIGHT) {
                    if(board[cursor.x][cursor.y].tank) {
                        rb->splash(HZ, "There is already a tank there");
                    } else {
                        board[cursor.x][cursor.y].tank = true;
                        humanres.cash -= 300;
                        humanres.tanks++;
                    }
                } else {
                    rb->splash(HZ, "Can't place men on enemy territory");
                }
            } else {
                rb->splash(HZ, "Not enough money!");
            }
            goto resources_menu;
            break;
        case 2:
            if(humanres.cash>=600) {
                rb->splash(HZ, "Where do you want to place the plane?");
                tempmenu = select_square();
                switch(tempmenu) {
                    case 0:
                        rb->splash(HZ, "Cancelled");
                        goto resources_menu;
                        break;
                    case PLUGIN_USB_CONNECTED:
                        return PLUGIN_USB_CONNECTED;
                        break;
                }
                if(board[cursor.x][cursor.y].colour == COLOUR_LIGHT) {
                    if(board[cursor.x][cursor.y].plane) {
                        rb->splash(HZ, "There is already a plane there");
                    } else {
                        board[cursor.x][cursor.y].plane = true;
                        humanres.cash -= 600;
                        humanres.planes++;
                    }
                } else {
                    rb->splash(HZ, "Can't place men on enemy territory");
                }
            } else {
                rb->splash(HZ, "Not enough money!");
            }
            goto resources_menu;
            break;
        case 3:
            if(humanres.cash>=1150) {
                rb->splash(HZ, "Where do you want to place the farm?");
                tempmenu = select_square();
                switch(tempmenu) {
                    case 0:
                        rb->splash(HZ, "Cancelled");
                        goto resources_menu;
                        break;
                    case PLUGIN_USB_CONNECTED:
                        return PLUGIN_USB_CONNECTED;
                        break;
                }
                if(board[cursor.x][cursor.y].colour == COLOUR_LIGHT) {
                    if(board[cursor.x][cursor.y].farm) {
                        rb->splash(HZ, "There is already a farm there");
                    } else {
                        board[cursor.x][cursor.y].farm = true;
                        humanres.cash -= 1150;
                        humanres.farms++;
                    }
                } else {
                    rb->splash(HZ, "Can't build on enemy territory");
                }
            } else {
                rb->splash(HZ, "Not enough money!");
            }
            goto resources_menu;
            break;
        case 4:
            if(humanres.cash>=1300) {
                rb->splash(HZ, "Where do you want to place the industrial" 
                               " plant?");
                tempmenu = select_square();
                switch(tempmenu) {
                    case 0:
                        rb->splash(HZ, "Cancelled");
                        goto resources_menu;
                        break;
                    case PLUGIN_USB_CONNECTED:
                        return PLUGIN_USB_CONNECTED;
                        break;
                }
                if(board[cursor.x][cursor.y].colour == COLOUR_LIGHT) {
                    if(board[cursor.x][cursor.y].ind) {
                        rb->splash(HZ, "There is already an industrial" 
                                       " plant there");
                    } else {
                        board[cursor.x][cursor.y].ind = true;
                        humanres.cash -= 1300;
                        humanres.inds++;
                    }
                } else {
                    rb->splash(HZ, "Can't build on enemy territory");
                }
            } else {
                rb->splash(HZ, "Not enough money!");
            }
            goto resources_menu;
            break;
        case 5:
            if(humanres.cash>=2000) {
                rb->splash(HZ, "Where do you want to place the nuke?");
                tempmenu = select_square();
                switch(tempmenu) {
                    case 0:
                        rb->splash(HZ, "Cancelled");
                        goto resources_menu;
                        break;
                    case PLUGIN_USB_CONNECTED:
                        return PLUGIN_USB_CONNECTED;
                        break;
                }
                if(board[cursor.x][cursor.y].colour == COLOUR_LIGHT) {
                    if(board[cursor.x][cursor.y].nuke) {
                        rb->splash(HZ, "There is already a nuke there");
                    } else {
                        board[cursor.x][cursor.y].nuke = true;
                        humanres.cash -= 2000;
                        humanres.nukes++;
                    }
                } else {
                    rb->splash(HZ, "Can't place a nuke on enemy territory");
                }
            } else {
                rb->splash(HZ, "Not enough money!");
            }
            goto resources_menu;
            break;
        case 6:
            return 0;
            break;
        case MENU_ATTACHED_USB:
            return PLUGIN_USB_CONNECTED;
            break;
    }
    return 0;
}

int move_unit(void) {
    int selection, nummen;
    struct cursor from;

    MENUITEM_STRINGLIST(move_unit_menu, "Move unit", NULL, "Move men", 
                    "Move tank", "Move plane");
    selection=rb->do_menu(&move_unit_menu,&selection, NULL, false);
    switch(selection) {
        case 0:
            rb->splash(HZ, "Select where to move troops from");
            if(select_square() == PLUGIN_USB_CONNECTED)
                return PLUGIN_USB_CONNECTED;
            if(board[cursor.x][cursor.y].colour == COLOUR_LIGHT) {
                if(board[cursor.x][cursor.y].men) {
                    from.x = cursor.x;
                    from.y = cursor.y;
                    nummen = board[from.x][from.y].men;
                    if(get_number("How many men do you want to move?", 
                                           &nummen) == PLUGIN_USB_CONNECTED)
                        return PLUGIN_USB_CONNECTED;
                    if(nummen > board[from.x][from.y].men) {
                        rb->splash(HZ, "You don't have that many troops.");
                    } else {
                        rb->splash(HZ,"Select where to move the troops to");
                        if(select_square() == PLUGIN_USB_CONNECTED)
                            return PLUGIN_USB_CONNECTED;
                        if((board[cursor.x][cursor.y].colour == COLOUR_LIGHT) && 
                                        (abs(cursor.x - from.x) <= 1) && 
                                        abs(cursor.y - from.y) <= 1) {
                            board[from.x][from.y].men -= nummen;
                            board[cursor.x][cursor.y].men += nummen;
                            humanres.moves--;
                            return 0;
                        }
                    }
                } else {
                    rb->splash(HZ, "You don't have any troops there");
                }
            } else {
                rb->splash(HZ, "Can't move enemy troops");
            }
            break;
        case 1:
            rb->splash(HZ, "Select where you want to move the tank from");
            if(select_square() == PLUGIN_USB_CONNECTED)
                return PLUGIN_USB_CONNECTED;
            if(board[cursor.x][cursor.y].colour == COLOUR_LIGHT) {
                if(board[cursor.x][cursor.y].tank) {
                    from.x = cursor.x;
                    from.y = cursor.y;
                    rb->splash(HZ, "Select where you want" 
                                   " to move the tank to");
                    if(select_square() == PLUGIN_USB_CONNECTED)
                        return PLUGIN_USB_CONNECTED;
                    if((board[cursor.x][cursor.y].colour == COLOUR_LIGHT)&& 
                                    (abs(cursor.x-from.x) <= 1) && 
                                    (abs(cursor.y-from.y) <= 1)) {
                        if(board[cursor.x][cursor.y].tank) {
                            rb->splash(HZ, "There is already a tank there");
                        } else {
                            board[from.x][from.y].tank = false;
                            board[cursor.x][cursor.y].tank = true;
                            humanres.moves--;
                            return 0;
                        }
                    } else {
                        rb->splash(HZ, "Invalid move");
                    }
                } else {
                    rb->splash(HZ, "You don't have a tank there");
                }
            } else {
                rb->splash(HZ, "That isn't your territory");
            }
            break;
        case 2:
            rb->splash(HZ, "Select where you want"
                           " to move the plane from");
            if(select_square() == PLUGIN_USB_CONNECTED)
                return PLUGIN_USB_CONNECTED;
            if(board[cursor.x][cursor.y].colour == COLOUR_LIGHT) {
                if(board[cursor.x][cursor.y].plane) {
                    from.x = cursor.x;
                    from.y = cursor.y;
                    rb->splash(HZ, "Select where you want" 
                                   " to move the plane to");
                    if(select_square() == PLUGIN_USB_CONNECTED)
                        return PLUGIN_USB_CONNECTED;
                    if(board[cursor.x][cursor.y].colour == COLOUR_LIGHT) {  
                        if(board[cursor.x][cursor.y].plane) {
                            rb->splash(HZ,"There is already a plane there");
                        } else {
                            board[from.x][from.y].plane = false;
                            board[cursor.x][cursor.y].plane = true;
                            humanres.moves--;
                            return 0;
                        }
                    } else {
                        rb->splash(HZ, "Invalid move");
                    }
                } else {
                    rb->splash(HZ, "You don't have a plane there");
                }
            } else {
                rb->splash(HZ, "That isn't your territory");
            }
            break;
    }
    return 0;
}

int movement_menu(void) {
    int selection, tempmenu;
    bool menu_quit = false;

    MENUITEM_STRINGLIST(move_menu, "Movement", NULL, "Move unit", 
                    "Buy additional moves ($100)", "Launch nuclear missile", 
                    "Check map", "Finish moving", "Game menu");

    while(!menu_quit) {
        selection=rb->do_menu(&move_menu,&selection, NULL, false);
        switch(selection) {
            case 0:
                if(humanres.moves) {
                    if(move_unit()==PLUGIN_USB_CONNECTED)
                        return PLUGIN_USB_CONNECTED;
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
                    if(select_square() == PLUGIN_USB_CONNECTED) {
                        return PLUGIN_USB_CONNECTED;
                    }
                    if(board[cursor.x][cursor.y].nuke) {
                        rb->splash(HZ, "Select place to target with nuke");
                        if(select_square() == PLUGIN_USB_CONNECTED) {
                            return PLUGIN_USB_CONNECTED;
                        }
                        board[cursor.x][cursor.y].men = 0;
                        board[cursor.x][cursor.y].tank = 0;
                        board[cursor.x][cursor.y].plane = 0;
                        board[cursor.x][cursor.y].ind = 0;
                        board[cursor.x][cursor.y].nuke = 0;
                        board[cursor.x][cursor.y].farm = 0;
                        /* TODO: Fallout carried by wind */
                    }
                }
                break;
            case 3:
                if(select_square() == PLUGIN_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
            case 4:
                return 0;
                break;
            case 5:
                tempmenu = ingame_menu();
                switch(tempmenu) {
                    case PLUGIN_USB_CONNECTED:
                        return PLUGIN_USB_CONNECTED;
                        break;
                    case SUPERDOM_QUIT:
                        return SUPERDOM_QUIT;
                        break;
                }
                break;
            case MENU_ATTACHED_USB:
                return PLUGIN_USB_CONNECTED;
                break;
        }
    }
    return 0;
}

int show_inventory(void) {
    rb->lcd_clear_display();
    rb->lcd_puts(1, 0, "Inventory");
    char men[20], tanks[20], planes[20], inds[20], farms[20], nukes[20], 
         cash[20], food[20], bank[20];
    rb->snprintf(men, sizeof(men), "Men: %d", humanres.men);
    rb->snprintf(tanks, sizeof(tanks), "Tanks: %d", humanres.tanks);
    rb->snprintf(planes, sizeof(planes), "Planes: %d", humanres.planes);
    rb->snprintf(inds, sizeof(inds), "Factories: %d", humanres.inds);
    rb->snprintf(farms, sizeof(farms), "Farms: %d", humanres.farms);
    rb->snprintf(nukes, sizeof(nukes), "Nukes: %d", humanres.nukes);
    rb->snprintf(cash, sizeof(cash), "Cash: %d", humanres.cash);
    rb->snprintf(food, sizeof(food), "Food: %d", humanres.food);
    rb->snprintf(bank, sizeof(bank), "Bank: %d", humanres.bank);
    rb->lcd_puts(2, 1, men);
    rb->lcd_puts(2, 2, tanks);
    rb->lcd_puts(2, 3, planes);
    rb->lcd_puts(2, 4, inds);
    rb->lcd_puts(2, 5, farms);
    rb->lcd_puts(2, 6, nukes);
    rb->lcd_puts(2, 7, cash);
    rb->lcd_puts(2, 8, food);
    rb->lcd_puts(2, 9, bank);
    rb->lcd_update();
    if(rb->default_event_handler(rb->button_get(true)) == SYS_USB_CONNECTED) {
        return PLUGIN_USB_CONNECTED;
    } else {
        return 0;
    }
}

int production_menu(void) {
    int selection, tempbank, tempmenu;

    MENUITEM_STRINGLIST(prod_menu, "Production", NULL, "Buy resources", 
                    "Show inventory", "Check map", "Invest money", 
                    "Withdraw money", "Finish turn", "Game menu");

    while(1) {
        selection=rb->do_menu(&prod_menu,&selection, NULL, false);
        switch(selection) {
            case 0:
                tempmenu = buy_resources_menu();
                switch(tempmenu) {
                    case PLUGIN_USB_CONNECTED:
                        return PLUGIN_USB_CONNECTED;
                        break;
                    case SUPERDOM_QUIT:
                        return SUPERDOM_QUIT;
                        break;
                }
                break;
            case 1:
                tempmenu = show_inventory();
                switch(tempmenu) {
                    case 0:
                        break;
                    case PLUGIN_USB_CONNECTED:
                        return PLUGIN_USB_CONNECTED;
                        break;
                }
                break;
            case 2:
                tempmenu = select_square();
                switch(tempmenu) {
                    case PLUGIN_USB_CONNECTED:
                        return PLUGIN_USB_CONNECTED;
                        break;
                    case SUPERDOM_QUIT:
                        return SUPERDOM_QUIT;
                        break;
                    case 0:
                        break;
                }
                break;
            case 3:
                tempbank = humanres.cash;
                if(get_number("How much do you want to invest?", &tempbank)
                                == PLUGIN_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                if(tempbank>humanres.cash) {
                    rb->splash(HZ, "You don't have that much cash to invest");
                } else {
                    humanres.cash -= tempbank;
                    humanres.bank += tempbank;
                }
                break;
            case 4:
                tempbank = 0;
                if(get_number("How much do you want to withdraw?", &tempbank)
                                == PLUGIN_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                if(tempbank>humanres.bank) {
                    rb->splash(HZ, "You don't have that much cash to withdraw");
                } else {
                    humanres.cash += tempbank;
                    humanres.bank -= tempbank;
                }
                break;
            case 5:
                return 0;
                break;
            case 6:
                tempmenu = ingame_menu();
                switch(tempmenu) {
                    case PLUGIN_USB_CONNECTED:
                        return PLUGIN_USB_CONNECTED;
                        break;
                    case SUPERDOM_QUIT:
                        return SUPERDOM_QUIT;
                        break;
                }
                break;
            case MENU_ATTACHED_USB:
                return PLUGIN_USB_CONNECTED;
                break;
        }
    }
    return 0;
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
    draw_board();
    draw_cursor();
    update_score();
#if LCD_WIDTH >= 220
    rb->snprintf(buf, sizeof(buf), "Cash: %d", humanres.cash);
    rb->lcd_putsxy(125, LCD_HEIGHT-20, buf);
    rb->snprintf(buf, sizeof(buf), "Food: %d", humanres.food);
    rb->lcd_putsxy(125, LCD_HEIGHT-10, buf);
#endif
    rb->lcd_update();
    int button = 0;
    while(1) {
        button = rb->button_get(true);
        switch(button) {
            case SUPERDOM_CANCEL:
                return 0;
                break;
            case SUPERDOM_OK:
                return 2;
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
                    return PLUGIN_USB_CONNECTED;
                }
        }
    }
}

int killmen(bool human) {
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
    for(i=1;i<12;i++) {
        for(j=1;j<12;j++) {
            if(board[i][j].colour == human) {
                menkilled += ((board[i][j].men * percent)/1000);
                board[i][j].men = (board[i][j].men * percent)/1000;
            }
        }
    }
        
    if(human)
        humanres.men -= menkilled;
    else
        compres.men -= menkilled;
    return menkilled;
}

int war_menu(void) {
    int selection, tempmenu;

    MENUITEM_STRINGLIST(wartime_menu, "War!", NULL, 
                    "Select territory to attack", "Finish turn", "Game menu");

    humanres.moves = superdom_settings.movesperturn;
    while(humanres.moves) {
        selection=rb->do_menu(&wartime_menu,&selection, NULL, false);
        switch(selection) {
            case 0:
                if(select_square() == PLUGIN_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                if(board[cursor.x][cursor.y].colour == COLOUR_DARK) {
                    if(calc_strength(COLOUR_LIGHT, cursor.x, 
                                     cursor.y) > calc_strength(COLOUR_DARK, 
                                     cursor.x, cursor.y)) {
                        board[cursor.x][cursor.y].colour = COLOUR_LIGHT;
                        board[cursor.x][cursor.y].tank = 0;
                        board[cursor.x][cursor.y].men = 0;
                        board[cursor.x][cursor.y].plane = 0;
                        board[cursor.x][cursor.y].nuke = 0;
                        draw_board();
                        rb->sleep(HZ*2);
                        humanres.moves--;
                    } else if(calc_strength(COLOUR_LIGHT, cursor.x, cursor.y)==
                              calc_strength(COLOUR_DARK, cursor.x, cursor.y)) {
                        if(rb->rand()%2) {
                            board[cursor.x][cursor.y].colour = COLOUR_LIGHT;
                            board[cursor.x][cursor.y].tank = 0;
                            board[cursor.x][cursor.y].men = 0;
                            board[cursor.x][cursor.y].plane = 0;
                            board[cursor.x][cursor.y].nuke = 0;
                            draw_board();
                            rb->sleep(HZ*2);
                            humanres.moves--;
                        } else {
                            rb->splash(HZ, "Your troops were unable to" 
                                           " overcome the enemy troops");
                            humanres.moves--;
                        }
                    } else {
                        rb->splash(HZ, "Your troops were unable to overcome"
                                       " the enemy troops");
                        humanres.moves--;
                    }
                } else {
                    rb->splash(HZ, "You can't attack your own territory");
                }
                break;
            case 1:
                return 0;
                break;
            case 2:
                tempmenu = ingame_menu();
                switch(tempmenu) {
                    case PLUGIN_USB_CONNECTED:
                        return PLUGIN_USB_CONNECTED;
                        break;
                    case SUPERDOM_QUIT:
                        return SUPERDOM_QUIT;
                        break;
                }
                break;
        }
    }
    return 0;
}

struct threat {
    int x;
    int y;
    int str_diff;
};

bool place_adjacent(bool tank, int x, int y) {
    if(tank) {
        if(!board[x-1][y].tank && (board[x][y].colour==board[x-1][y].colour)) {
            compres.cash -= 300;
            board[x-1][y].tank = true;
            compres.tanks++;
            return 0;
        }
        if(!board[x+1][y].tank && (board[x][y].colour==board[x+1][y].colour)) {
            compres.cash -= 300;
            board[x+1][y].tank = true;
            compres.tanks++;
            return 0;
        }
        if(!board[x][y-1].tank && (board[x][y].colour==board[x][y-1].colour)) {
            compres.cash -= 300;
            board[x][y-1].tank = true;
            compres.tanks++;
            return 0;
        }
        if(!board[x][y+1].tank && (board[x][y].colour==board[x][y+1].colour)) {
            compres.cash -= 300;
            board[x][y+1].tank = true;
            compres.tanks++;
            return 0;
        }
    } else {
        if(!board[x-1][y].plane && (board[x][y].colour==board[x-1][y].colour)) {
            compres.cash -= 600;
            board[x-1][y].plane = true;
            compres.planes++;
            return 0;
        }
        if(!board[x+1][y].plane && (board[x][y].colour==board[x+1][y].colour)) {
            compres.cash -= 600;
            board[x+1][y].plane = true;
            compres.planes++;
            return 0;
        }
        if(!board[x][y-1].plane && (board[x][y].colour==board[x][y-1].colour)) {
            compres.cash -= 600;
            board[x][y-1].plane = true;
            compres.planes++;
            return 0;
        }
        if(!board[x][y+1].plane && (board[x][y].colour==board[x][y+1].colour)) {
            compres.cash -= 600;
            board[x][y+1].plane = true;
            compres.planes++;
            return 0;
        }
    }
    return 1;
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

void find_adjacent(int x, int y, int* adj_x, int* adj_y, bool* full) {
    /* Finds adjacent squares, returning squares without tanks on them
     * in preference to those with them */
    if(((board[x-1][y].tank && (board[x-1][y].colour == COLOUR_DARK)) ||
        board[x-1][y].colour != COLOUR_DARK) &&
       ((board[x+1][y].tank && (board[x+1][y].colour == COLOUR_DARK)) ||
        board[x+1][y].colour != COLOUR_DARK) &&
       ((board[x][y-1].tank && (board[x][y-1].colour == COLOUR_DARK)) ||
        board[x][y-1].colour != COLOUR_DARK) &&
       ((board[x][y+1].tank && (board[x][y+1].colour == COLOUR_DARK)) || 
        board[x][y+1].colour != COLOUR_DARK)) {
        *full = true;
    } else {
        *full = false;
    }

    if(board[x-1][y].colour == COLOUR_DARK) {
        *adj_x = x-1;
        *adj_y = y;
        if(board[x-1][y].tank) {
            if(*full)
                return;
        } else {
            return;
        }
    }
    if(board[x+1][y].colour == COLOUR_DARK) {
        *adj_x = x+1;
        *adj_y = y;
        if(board[x+1][y].tank) {
            if(*full)
                return;
        } else {
            return;
        }
    }
    if(board[x][y-1].colour == COLOUR_DARK) {
        *adj_x = x;
        *adj_y = y-1;
        if(board[x][y-1].tank) {
            if(*full)
                return;
        } else {
            return;
        }
    }
    if(board[x][y+1].colour == COLOUR_DARK) {
        *adj_x = x;
        *adj_y = y+1;
        if(board[x][y+1].tank) {
            if(*full)
                return;
        } else {
            return;
        }
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
    int men_needed;
    struct threat targets[2];
    int numtargets;
    struct cursor adj;
    bool full = false;
    for(i=1;i<12;i++) {
        for(j=1;j<12;j++) {
            if((board[i][j].colour == COLOUR_DARK) && 
               (calc_strength(COLOUR_DARK,i,j) < 
                calc_strength(COLOUR_LIGHT,i,j))) {
                if(board[i][j].ind || board[i][j].farm) {
                    if(numthreats < 3) {
                        offensive = false;
                        threats[numthreats].x = i;
                        threats[numthreats].y = j;
                        threats[numthreats].str_diff = 
                                calc_strength(COLOUR_LIGHT,i,j) - 
                                calc_strength(COLOUR_DARK,i,j);
                        numthreats++;
                    }
                }
            }
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
        while(k<numtargets) {
            for(i=1;i<12;i++) {
                for(j=1;j<12;j++) {
                    if((board[i][j].colour == COLOUR_LIGHT) && 
                       (board[i][j].ind || board[i][j].farm) &&
                        has_adjacent(i,j)) {
                        targets[k].x = i;
                        targets[k].y = j;
                        targets[k].str_diff = abs(calc_strength(COLOUR_LIGHT, 
                                              i, j) - calc_strength(COLOUR_DARK,
                                              i, j));
                        k++;
                    }
                }
            }
        }
        if(k == 0) {
            /* No targets found! Randomly pick squares and if they're owned 
             * by the computer then stick a tank on it. */
                rb->srand(*rb->current_tick);
                while(compres.cash >= 300) {
                    i = rb->rand()%11 + 1;
                    j = rb->rand()%11 + 1;
                    if(board[i][j].colour == COLOUR_DARK) {
                        if(compres.cash >= 300) {
                            if(!board[i][j].tank) {
                                board[i][j].tank = true;
                                compres.tanks++;
                                compres.cash -= 300;
                                draw_board();
                                rb->sleep(HZ);
                            }
                        }
                    }
                }
                compres.bank += compres.cash;
                compres.cash = 0;
        } else {
            for(i=0;i<k;i++) {
                men_needed = targets[i].str_diff + 20;
                find_adjacent(targets[i].x,targets[i].y, &adj.x, &adj.y, &full);
                while(((calc_strength(COLOUR_LIGHT, targets[i].x, targets[i].y)
                       + 20) > calc_strength(COLOUR_DARK, targets[i].x, 
                               targets[i].y)) && compres.cash > 0) {
                    /* While we still need them keep placing men */
                    if(compres.cash >= 300 && !full) {
                        if(board[adj.x][adj.y].tank) {
                            find_adjacent(targets[i].x, targets[i].y, 
                                            &adj.x, &adj.y, &full);
                        } else {
                            board[adj.x][adj.y].tank = true;
                            compres.tanks++;
                            compres.cash -= 300;
                            draw_board();
                            rb->sleep(HZ);
                        }
                    } else {
                        men_needed = (calc_strength(COLOUR_LIGHT, targets[i].x,
                                      targets[i].y) + 20 - 
                                      calc_strength(COLOUR_DARK, targets[i].x, 
                                      targets[i].y))*1000/133;
                        if(compres.cash >= men_needed) {
                            board[adj.x][adj.y].men += men_needed;
                            compres.men += men_needed;
                            compres.cash -= men_needed;
                            compres.bank += compres.cash;
                            compres.cash = 0;
                        } else {
                            board[adj.x][adj.y].men += compres.cash;
                            compres.men += compres.cash;
                            compres.cash = 0;
                        }
                        draw_board();
                        rb->sleep(HZ);
                    }
                }
            }
            compres.bank += compres.cash;
            compres.cash = 0;
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
                if(compres.cash >= men_needed) {
                    board[threats[i].x][threats[i].y].men += men_needed;
                    compres.cash -= men_needed;
                    compres.men += men_needed;
                    draw_board();
                    rb->sleep(HZ);
                } else {
                    board[threats[i].x][threats[i].y].men += compres.cash;
                    compres.men += compres.cash;
                    compres.cash = 0;
                    draw_board();
                    rb->sleep(HZ);
                }
            }
        } else if((total_str_diff+20)*15 < compres.cash) {
            /* Enough money to pay their way by planes */
            for(i=0;i<numthreats;i++) {
                while(calc_strength(COLOUR_DARK,threats[i].x, threats[i].y) < 
                      (calc_strength(COLOUR_LIGHT,threats[i].x, threats[i].y) +
                       20)) {
                    if(board[threats[i].x][threats[i].y].plane) {
                        if(place_adjacent(0, threats[i].x, threats[i].y)) {
                            /* No room for any more planes, revert to men */
                            men_needed = (calc_strength(COLOUR_LIGHT, 
                                         threats[i].x, threats[i].y) + 20 - 
                                         calc_strength(COLOUR_DARK, 
                                         threats[i].x, threats[i].y)*1000/133);
                            if(compres.cash >= men_needed) {
                                compres.cash -= men_needed;
                                compres.men += men_needed;
                                board[threats[i].x][threats[i].y].men += 
                                        men_needed;
                                draw_board();
                                rb->sleep(HZ);
                            }
                        }
                    } else {
                        if(compres.cash >= 600) {
                            board[threats[i].x][threats[i].y].plane = true;
                            compres.cash -= 600;
                            compres.planes++;
                            draw_board();
                            rb->sleep(HZ);
                        }
                    }
                }
            }
        } else {
            /* Tanks it is */
            for(i=0;i<numthreats;i++) {
                while(calc_strength(COLOUR_DARK,threats[i].x, threats[i].y) < 
                      (calc_strength(COLOUR_LIGHT,threats[i].x, threats[i].y) +
                       20) && compres.cash > 0) {
                    if(board[threats[i].x][threats[i].y].tank) {
                        if(place_adjacent(1, threats[i].x, threats[i].y)) {
                            /* No room for any more tanks, revert to men */
                            men_needed = (calc_strength(COLOUR_LIGHT, 
                                         threats[i].x, threats[i].y) + 20 - 
                                         calc_strength(COLOUR_DARK, 
                                         threats[i].x, threats[i].y)*1000/133);
                            if(compres.cash >= men_needed) {
                                compres.cash -= men_needed;
                                compres.men += men_needed;
                                board[threats[i].x][threats[i].y].men += 
                                        men_needed;
                                draw_board();
                                rb->sleep(HZ);
                            }
                        }
                    } else {
                        if(compres.cash >= 300) {
                            board[threats[i].x][threats[i].y].tank = true;
                            compres.tanks++;
                            compres.cash -= 300;
                            draw_board();
                            rb->sleep(HZ);
                        }
                    }
                }
            }
        }
        compres.bank += compres.cash;
        compres.cash = 0;
    }
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
    struct cursor adj;
    
    while(compres.moves) {
        for(i=1;i<12;i++) {
            for(j=1;j<12;j++) {
                if((board[i][j].colour == COLOUR_DARK) &&
                   (board[i][j].farm || board[i][j].ind)) { 
                    if(find_adj_target(i, j, &adj) && compres.moves) {
                        if(calc_strength(COLOUR_LIGHT, adj.x, adj.y) == 
                                  calc_strength(COLOUR_DARK, adj.x, adj.y)) {
                                rb->srand(*rb->current_tick);
                                if(rb->rand()%2) {
                                    board[adj.x][adj.y].colour = COLOUR_DARK;
                                    board[adj.x][adj.y].tank = false;
                                    board[adj.x][adj.y].plane = false;
                                    board[adj.x][adj.y].nuke = false;
                                    humanres.men -= board[adj.x][adj.y].men;
                                    board[adj.x][adj.y].men = 0;
                                    draw_board();
                                    rb->sleep(HZ);
                                    compres.moves--;
                                } else {
                                    rb->splash(HZ*2, "The computer attempted"
                                                     " to attack, but the"
                                                     " invasion was pushed"
                                                     " back");
                                    compres.moves--;
                                }
                        } else {
                            if(compres.moves) {
                                board[adj.x][adj.y].colour = COLOUR_DARK;
                                board[adj.x][adj.y].tank = false;
                                board[adj.x][adj.y].plane = false;
                                board[adj.x][adj.y].nuke = false;
                                humanres.men -= board[adj.x][adj.y].men;
                                board[adj.x][adj.y].men = 0;
                                draw_board();
                                rb->sleep(HZ);
                                compres.moves--;
                            }
                        }
                    }
                }
            }
        }
        if(compres.moves) {
            /* Defence stage done, move on to OFFENCE */
            for(i=1;i<12;i++) {
                for(j=1;j<12;j++) {
                    if(board[i][j].colour == COLOUR_LIGHT && compres.moves &&
                            (board[i][j].ind || board[i][j].farm) &&
                            (calc_strength(COLOUR_DARK, i, j) >=
                             calc_strength(COLOUR_LIGHT, i, j))) {
                        if(calc_strength(COLOUR_DARK, i, j) ==
                           calc_strength(COLOUR_LIGHT, i, j)) {
                            if(rb->rand()%2) {
                                board[i][j].colour = COLOUR_DARK;
                                board[i][j].tank = false;
                                board[i][j].plane = false;
                                board[i][j].nuke = false;
                                board[i][j].men = 0;
                                draw_board();
                                rb->sleep(HZ);
                                compres.moves--;
                            } else {
                                rb->splash(HZ*2, "The computer attempted to " 
                                                "attack, but the invasion was"
                                                " pushed back");
                                compres.moves--;
                            }
                        } else {
                            board[i][j].colour = COLOUR_DARK;
                            board[i][j].tank = false;
                            board[i][j].plane = false;
                            board[i][j].nuke = false;
                            board[i][j].men = 0;
                            draw_board();
                            rb->sleep(HZ);
                            compres.moves--;
                        }
                    }
                }
            }
            while(compres.moves > 0) {
                /* Spend leftover moves wherever attacking randomly */
                rb->srand(*rb->current_tick);
                i = (rb->rand()%10)+1;
                j = (rb->rand()%10)+1;
                if(board[i][j].colour == COLOUR_LIGHT && 
                  (calc_strength(COLOUR_DARK, i, j)  >= 
                   calc_strength(COLOUR_LIGHT, i, j))) {
                    if(calc_strength(COLOUR_DARK, i, j) == 
                                    calc_strength(COLOUR_LIGHT, i, j)) {
                        if(rb->rand()%2) {
                            board[i][j].colour = COLOUR_DARK;
                            board[i][j].tank = false;
                            board[i][j].plane = false;
                            board[i][j].nuke = false;
                            board[i][j].men = 0;
                            draw_board();
                            rb->sleep(HZ);
                            compres.moves--;
                        } else {
                            rb->splash(HZ*2, "The computer attempted to" 
                                             " attack, but the invasion was"
                                             " pushed back");
                            compres.moves--;
                        }
                    } else {
                        board[i][j].colour = COLOUR_DARK;
                        board[i][j].tank = false;
                        board[i][j].plane = false;
                        board[i][j].nuke = false;
                        board[i][j].men = 0;
                        draw_board();
                        rb->sleep(HZ);
                        compres.moves--;
                    }
                }
            }
        }
    }
}

int load_game(char* file) {
    int fd;

    fd = rb->open(file, O_RDONLY);
    if(fd == 0) {
        DEBUGF("Couldn't open savegame\n");
        return -1;
    }
    rb->read(fd, buf, 5);
    if(rb->strcmp(buf, "SSGv2")) {
        rb->splash(HZ, "Invalid/incompatible savegame\n");
        return -1;
    }
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
    rb->read(fd, &superdom_settings.humanstartfarms, sizeof(int));
    rb->read(fd, &superdom_settings.startcash, sizeof(int));
    rb->read(fd, &superdom_settings.startfood, sizeof(int));
    rb->read(fd, &superdom_settings.movesperturn, sizeof(int));
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

int average_strength(bool colour) {
    /* This function calculates the average strength of the given player,
     * used to determine when the computer wins or loses. */
    int i,j;
    int totalpower = 0;
    for(i=0;i<12;i++) {
        for(j=0;j<12;j++) {
            if(board[i][j].colour != -1) {
                totalpower += calc_strength(colour, i, j);
            }
        }
    }
    return totalpower/100;
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int tempmenu;
    bool statusbar_setting;

    rb = api;

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_set_background(LCD_WHITE);
#endif

    statusbar_setting = rb->global_settings->statusbar; 
    rb->global_settings->statusbar = false;
    cursor.x = 1;
    cursor.y = 1;
    default_settings();
    if(parameter) {
        if(load_game(parameter) != 0) {
            DEBUGF("Loading failed, generating new game\n");
            init_resources();
            init_board();
        } else {
            goto startyear;
        }
    } else {
        init_resources();
        init_board();
    }

    bool play = false;
    while(!play) {
        switch(menu()) {
            case 0:
                play = true;
                break;
            case 2:
                rb->global_settings->statusbar = statusbar_setting; 
                return PLUGIN_OK;
                break;
        }
    }
    gen_resources();
startyear:
    if((average_strength(COLOUR_LIGHT) - average_strength(COLOUR_DARK)) > 15) {
        rb->splash(HZ*4, "The computer has surrendered. You win.");
        rb->global_settings->statusbar = statusbar_setting; 
        return PLUGIN_OK;
    }
    if((average_strength(COLOUR_DARK) - average_strength(COLOUR_LIGHT)) > 15) {
        rb->splash(HZ*4, "Your army have suffered terrible morale from the bleak prospects of winning. You lose");
        rb->global_settings->statusbar = statusbar_setting; 
        return PLUGIN_OK;
    }
    tempmenu = production_menu();
    switch(tempmenu) {
        case PLUGIN_USB_CONNECTED:
            rb->global_settings->statusbar = statusbar_setting; 
            return PLUGIN_USB_CONNECTED;       
            break;
        case SUPERDOM_QUIT:
            rb->global_settings->statusbar = statusbar_setting; 
            return PLUGIN_OK;
            break;
    }
    computer_allocate();
    humanres.moves += superdom_settings.movesperturn;
    tempmenu = movement_menu();
    switch(tempmenu) {
        case PLUGIN_USB_CONNECTED:
            rb->global_settings->statusbar = statusbar_setting; 
            return PLUGIN_USB_CONNECTED;       
            break;
        case SUPERDOM_QUIT:
            rb->global_settings->statusbar = statusbar_setting; 
            return PLUGIN_OK;
            break;
    }
    if(humanres.men) {
        if(humanres.food > humanres.men) {
            rb->snprintf(buf, sizeof(buf), "Your men ate %d units of food", 
                            humanres.men);
            humanres.food -= humanres.men;
        } else {
            rb->snprintf(buf, sizeof(buf), "There was not enough food to feed" 
                            " all your men, %d men have died of starvation", 
                            killmen(COLOUR_LIGHT));
        }
        rb->splash(HZ*2, buf);
    }
    if(compres.men) {
        if(compres.food < compres.men) {
            rb->snprintf(buf, sizeof(buf), "The computer does not have enough"
                            " food to feed its men. %d have ided of starvation",
                            killmen(COLOUR_DARK));
            rb->splash(HZ, buf);
        }
    }
    tempmenu = war_menu();
    switch(tempmenu) {
        case PLUGIN_USB_CONNECTED:
            rb->global_settings->statusbar = statusbar_setting; 
            return PLUGIN_USB_CONNECTED;       
            break;
        case SUPERDOM_QUIT:
            rb->global_settings->statusbar = statusbar_setting; 
            return PLUGIN_OK;
            break;
    }
    compres.moves += superdom_settings.movesperturn;
    computer_war();
    gen_resources();
    goto startyear;
    rb->global_settings->statusbar = statusbar_setting; 
    return PLUGIN_OK;
}
