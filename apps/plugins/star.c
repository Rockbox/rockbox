/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Frederic Dang Ngoc
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#ifdef HAVE_LCD_BITMAP

PLUGIN_HEADER

/* file which contains the levels */
#define STAR_LEVELS_FILE "/.rockbox/star/levels.txt"

/* title of the game */
#define STAR_TITLE      "Star"

/* font used to display title */
#define STAR_TITLE_FONT  2

/* size of a level in file */
#define STAR_LEVEL_SIZE    ((STAR_WIDTH + 1) * STAR_HEIGHT + 1)

/* size of the game board */
#define STAR_WIDTH      16
#define STAR_HEIGHT      9

/* left and top margin */
#define STAR_OFFSET_X    8
#define STAR_OFFSET_Y    0

/* number of level */
#define STAR_LEVEL_COUNT 20

/* size of a tile */
#define STAR_TILE_SIZE   6

/* values of object in the board */
#define STAR_VOID       '.' 
#define STAR_WALL       '*' 
#define STAR_STAR       'o' 
#define STAR_BALL       'X' 
#define STAR_BLOCK      'x' 

/* sleep time between two frames */
#define STAR_SLEEP      1

/* value of ball and block control */
#define STAR_CONTROL_BALL  0
#define STAR_CONTROL_BLOCK 1

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define STAR_QUIT BUTTON_OFF
#define STAR_UP   BUTTON_UP
#define STAR_DOWN BUTTON_DOWN
#define STAR_TOGGLE_CONTROL BUTTON_ON
#define STAR_TOGGLE_CONTROL2 BUTTON_PLAY
#define STAR_LEVEL_UP BUTTON_F3
#define STAR_LEVEL_DOWN BUTTON_F1
#define STAR_LEVEL_REPEAT BUTTON_F2
#define STAR_MENU_RUN BUTTON_PLAY
#define STAR_MENU_RUN2 BUTTON_RIGHT
#define STAR_MENU_RUN3 BUTTON_ON

#elif CONFIG_KEYPAD == ONDIO_PAD
#define STAR_QUIT BUTTON_OFF
#define STAR_UP   BUTTON_UP
#define STAR_DOWN BUTTON_DOWN
#define STAR_TOGGLE_CONTROL_PRE BUTTON_MENU
#define STAR_TOGGLE_CONTROL (BUTTON_MENU | BUTTON_REL)
#define STAR_LEVEL_UP (BUTTON_MENU | BUTTON_RIGHT)
#define STAR_LEVEL_DOWN (BUTTON_MENU | BUTTON_LEFT)
#define STAR_LEVEL_REPEAT (BUTTON_MENU | BUTTON_UP)
#define STAR_MENU_RUN BUTTON_RIGHT

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define STAR_QUIT BUTTON_OFF
#define STAR_UP   BUTTON_UP
#define STAR_DOWN BUTTON_DOWN
#define STAR_TOGGLE_CONTROL_PRE BUTTON_MODE
#define STAR_TOGGLE_CONTROL (BUTTON_MODE | BUTTON_REL)
#define STAR_LEVEL_UP (BUTTON_MODE | BUTTON_RIGHT)
#define STAR_LEVEL_DOWN (BUTTON_MODE | BUTTON_LEFT)
#define STAR_LEVEL_REPEAT (BUTTON_MODE | BUTTON_UP)
#define STAR_MENU_RUN BUTTON_RIGHT

#elif (CONFIG_KEYPAD == IPOD_4G_PAD)

#define STAR_QUIT (BUTTON_SELECT | BUTTON_MENU)
#define STAR_UP   BUTTON_SCROLL_BACK
#define STAR_DOWN BUTTON_SCROLL_FWD
#define STAR_TOGGLE_CONTROL_PRE BUTTON_MENU
#define STAR_TOGGLE_CONTROL (BUTTON_MENU | BUTTON_REL)
#define STAR_LEVEL_UP (BUTTON_SELECT | BUTTON_RIGHT)
#define STAR_LEVEL_DOWN (BUTTON_SELECT | BUTTON_LEFT)
#define STAR_LEVEL_REPEAT (BUTTON_SELECT | BUTTON_PLAY)
#define STAR_MENU_RUN BUTTON_RIGHT

#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)

#define STAR_QUIT BUTTON_POWER
#define STAR_UP   BUTTON_UP
#define STAR_DOWN BUTTON_DOWN
#define STAR_TOGGLE_CONTROL_PRE BUTTON_SELECT
#define STAR_TOGGLE_CONTROL (BUTTON_SELECT | BUTTON_REL)
#define STAR_LEVEL_UP (BUTTON_PLAY | BUTTON_UP)
#define STAR_LEVEL_DOWN (BUTTON_PLAY | BUTTON_DOWN)
#define STAR_LEVEL_REPEAT (BUTTON_PLAY | BUTTON_RIGHT)
#define STAR_MENU_RUN BUTTON_REC

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)

#define STAR_QUIT BUTTON_A
#define STAR_UP   BUTTON_UP
#define STAR_DOWN BUTTON_DOWN
#define STAR_TOGGLE_CONTROL_PRE BUTTON_MENU
#define STAR_TOGGLE_CONTROL (BUTTON_MENU | BUTTON_REL)
#define STAR_LEVEL_UP (BUTTON_POWER | BUTTON_UP)
#define STAR_LEVEL_DOWN (BUTTON_POWER | BUTTON_DOWN)
#define STAR_LEVEL_REPEAT (BUTTON_POWER | BUTTON_RIGHT)
#define STAR_MENU_RUN BUTTON_RIGHT

#endif

/* function returns because of USB? */
static bool usb_detected = false;

/* position of the ball */
static int ball_x, ball_y;

/* position of the block */
static int block_x, block_y;

/* number of stars to get to finish the level */
static int star_count;

/* the object we control : ball or block */
static int control;

/* the current board */
static char board[STAR_HEIGHT][STAR_WIDTH];

/* bitmap of the wall */
static unsigned char wall_bmp[STAR_TILE_SIZE]
    = {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55};

/* bitmap of the star */
static unsigned char star_bmp[STAR_TILE_SIZE]
    = {0x00, 0x0c, 0x12, 0x12, 0x0c, 0x00};

/* bitmap of the ball */
static unsigned char ball_bmp[STAR_TILE_SIZE]
    = {0x00, 0x0c, 0x1e, 0x1a, 0x0c, 0x00};

/* bitmap of the block */
static unsigned char block_bmp[STAR_TILE_SIZE]
    = {0x00, 0x1e, 0x1e, 0x1e, 0x1e, 0x00};

/* bitmap of the arrow animation */
static unsigned char arrow_bmp[4][7] =
    {
        {0x7f, 0x7f, 0x3e, 0x3e, 0x1c, 0x1c, 0x08},
        {0x3e, 0x3e, 0x1c, 0x1c, 0x08, 0x08, 0x08},
        {0x1c, 0x1c, 0x1c, 0x1c, 0x08, 0x08, 0x08},
        {0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08}
    };

/* sequence of the bitmap arrow to follow to do one turn */
static unsigned char anim_arrow[8] = {0, 1, 2, 3, 2, 1, 0};

/* current_level */
static int current_level = 0;

/* char font size */
static int char_width = -1;
static int char_height = -1;

static struct plugin_api* rb;

/* this arrays contains a group of levels loaded into memory */
static unsigned char* levels = 
"****************\n"
"*X**........o*x*\n"
"*..........o.***\n"
"*.......**o....*\n"
"*...**.o......**\n"
"**.o..o.....o..*\n"
"*.o......o**.o.*\n"
"*.....**o.....o*\n"
"****************\n"
"\n"
".*..*.*.*...*.**\n"
"*...o.........X*\n"
"...*o..*o...o...\n"
"*.*.o.....o..*.*\n"
"......*...o...*.\n"
"*....*x*..o....*\n"
"...*..*.*...*oo*\n"
"*.............*.\n"
".*..*........*..\n"
"\n"
"****************\n"
"*...........o*x*\n"
"*...**......**X*\n"
"*...*o.........*\n"
"*.o.....o**...o*\n"
"*.*o..o..o*..o**\n"
"*.**o.*o..o.o***\n"
"*o....**o......*\n"
"****************\n"
"\n"
"****************\n"
"*............*x*\n"
"*.....*........*\n"
"**o*o.o*o*o*o*o*\n"
"*.*.*o.o*.*.*.**\n"
"**o*o*o.o*o*o*o*\n"
"*.....*........*\n"
"*...*.......*X.*\n"
"****************\n"
"\n"
".**************.\n"
"*X..*...*..*...*\n"
"*..*o.*.o..o.*.*\n"
"**......*..*...*\n"
"*o.*o*........**\n"
"**.....*.o.*...*\n"
"*o*..*.*.*...*x*\n"
"*...*....o*..*o*\n"
".**************.\n"
"\n"
"....************\n"
"...*...o...*o.o*\n"
"..*....o....*.**\n"
".*.....o.......*\n"
"*X.....o.......*\n"
"**.....o..*...**\n"
"*......o....*..*\n"
"*x.*...o..**o..*\n"
"****************\n"
"\n"
"****************\n"
"*..............*\n"
".**.***..*o.**o*\n"
".*o..*o.*.*.*.*.\n"
"..*..*..***.**..\n"
".**..*..*o*.*o**\n"
"*..............*\n"
"*..X*o....x..*o*\n"
"****************\n"
"\n"
"***************.\n"
"*..o**.........*\n"
"*..*o..**.o....*\n"
"*..o**.*.*o....*\n"
"**.....**..*o*.*\n"
"**.*.......o*o.*\n"
"*oxo*...o..*X*.*\n"
"**.............*\n"
".***************\n"
"\n"
"..*.***********.\n"
".*o*o......*..X*\n"
"*o.o*....o....*.\n"
".*.*..o**..o*..*\n"
"*..*o.*oxo....o*\n"
"*.....o**.....*.\n"
"*o*o.........*..\n"
"*...........*...\n"
".***********....\n"
"\n"
"....***********.\n"
"*****.o........*\n"
"*...x.***o.o*.o*\n"
"*.o...*o.*o...*.\n"
"*.....*..o..*.o*\n"
"*o*o..*.o*..*X*.\n"
".*o...***..***..\n"
"*.........*.*.*.\n"
".*********..*..*\n"
"\n"
"****************\n"
"*......*......X*\n"
"*..*oo.....oo.**\n"
"**...o...**...o*\n"
"*o....*o*oo..***\n"
"**.**....**....*\n"
"*o..o*.o....x.o*\n"
"**o***....*...**\n"
"***************.\n"
"\n"
"**.....**..****.\n"
"*X*****o.***.o**\n"
"*....oo.....o..*\n"
"*.**..**o..*o*.*\n"
"*.*.o.*.*o.**..*\n"
"*.**..**...*x*.*\n"
"*.....o........*\n"
"*........o.....*\n"
"****************\n"
"\n"
".**************.\n"
"*.X*........o.**\n"
"*.*...*o...o**.*\n"
"*.......o....*.*\n"
"*.o..........*o*\n"
"*.*......o.....*\n"
"**......o.o..*o*\n"
"*x..*....o.*.*.*\n"
".**************.\n"
"\n"
"****************\n"
"*o*o........o*o*\n"
"*.o*X......**..*\n"
"*.x........o...*\n"
"*........o*....*\n"
"*......o.......*\n"
"*.o*........*..*\n"
"*o*o........o*o*\n"
"****************\n"
"\n"
".******.********\n"
"*.....o*.....o.*\n"
"*.*.o.*..*...o.*\n"
"*..X*...*oo.*o.*\n"
".*.*...*.o..x*.*\n"
"*o.......*..*o.*\n"
".*......o.....*.\n"
"*o............o*\n"
"****************\n"
"\n"
"****************\n"
"**.x*o.o......o*\n"
"*o.Xo*o.......**\n"
"**.***........**\n"
"**.....o*o*....*\n"
"*oo.......o*o..*\n"
"**.o....****o..*\n"
"**o*..*........*\n"
"****************\n"
"\n"
"****************\n"
"*.o*........*X.*\n"
"*.*..o*oo*o..*.*\n"
"*....*o**o*.o..*\n"
"*.o*.......o*..*\n"
"*..o*o....o*...*\n"
"*.*..*.**o*..*.*\n"
"*....o.*o...x..*\n"
"****************\n"
"\n"
"****************\n"
"*.o....o..x*...*\n"
"*..*o*o...*o...*\n"
"*...*o*....*o..*\n"
"*...o..*...o*o.*\n"
"*.*o*...*.o*...*\n"
"*.o*o.*.o.*....*\n"
"*o*X..*.....*..*\n"
"****************\n"
"\n"
"****************\n"
"*o...**.....**o*\n"
"*.*..*......*o.*\n"
"*.o*...o**..o..*\n"
"*.*....*o......*\n"
"*....*...o*....*\n"
"*.**.o*.**o..*x*\n"
"*.o*.*o.....**X*\n"
"****************\n"
"\n"
"****************\n"
"*...o*o........*\n"
"**o..o*.**o...**\n"
"*.*.*.o...*..*.*\n"
"*.x.*..**..*.Xo*\n"
"*.*..*...o.*.*.*\n"
"**...o**.*o..o**\n"
"*........o*o...*\n"
"****************";

/**
 * Display text.
 */
static void star_display_text(char *str, bool waitkey)
{
    int chars_by_line;
    int lines_by_screen;
    int chars_for_line;
    int current_line = 0;
    int first_char_index = 0;
    char *ptr_char;
    char *ptr_line;
    int i;
    char line[255];
    int key;
    bool go_on;

    rb->lcd_clear_display();

    chars_by_line = LCD_WIDTH / char_width;
    lines_by_screen = LCD_HEIGHT / char_height;

    do
    {
        ptr_char = str + first_char_index;
        chars_for_line = 0;
        i = 0;
        ptr_line = line;
        while (i < chars_by_line)
        {
            switch (*ptr_char)
            {
                case '\t':
                case ' ':
                    *(ptr_line++) = ' ';
                case '\n':
                case '\0':
                    chars_for_line = i;
                    break;

                default:
                    *(ptr_line++) = *ptr_char;
            }
            if (*ptr_char == '\n' || *ptr_char == '\0')
                break;
            ptr_char++;
            i++;
        }

        if (chars_for_line == 0)
            chars_for_line = i;

        line[chars_for_line] = '\0';

        // test if we have cutted a word. If it is the case we don't have to
        // skip the space
        if (i == chars_by_line && chars_for_line == chars_by_line)
            first_char_index += chars_for_line;
        else
            first_char_index += chars_for_line + 1;

        // print the line on the screen
        rb->lcd_putsxy(0, current_line * char_height, line);

        // if the number of line showed on the screen is equals to the
        // maximum number of line we can show, we wait for a key pressed to
        // clear and show the remaining text.
        current_line++;
        if (current_line == lines_by_screen || *ptr_char == '\0')
        {
            current_line = 0;
            rb->lcd_update();
            go_on = false;
            while (waitkey && !go_on)
            {
                key = rb->button_get(true);
                switch (key)
                {
                    case STAR_MENU_RUN:
                    case STAR_DOWN:
                        go_on = true;
                        break;
                        
                    default:
                        if (rb->default_event_handler(key) == SYS_USB_CONNECTED)
                        {
                            usb_detected = true;
                            go_on = true;
                            break;
                        }
                }
            }
            rb->lcd_clear_display();
        }
    } while (*ptr_char != '\0');
}

/**
 * Do a pretty transition from one level to another.
 */
static void star_transition_update(void)
{ 
    int center_x = LCD_WIDTH / 2;
    int lcd_demi_width = LCD_WIDTH / 2;
    int center_y = LCD_HEIGHT / 2;
    int x;
    int y = 0;
    int var_y = 0;

    for (x = 1 ; x < lcd_demi_width ; x++)
    {
        var_y += LCD_HEIGHT;
        if (var_y > LCD_WIDTH)
        {
            var_y -= LCD_WIDTH;
            y++;
        }
        rb->lcd_update_rect(center_x - x, center_y - y,
            x * 2, y * 2);
        rb->sleep(STAR_SLEEP);
    }
    rb->lcd_update();
}

/**
 * Display information board of the current level.
 */
static void star_display_board_info(void)
{
    int label_offset_y = label_offset_y = LCD_HEIGHT - char_height;
    char str_info[32];

    rb->snprintf(str_info, sizeof(str_info), "L:%02d    S:%02d   C:",
                 current_level, star_count);
    rb->lcd_putsxy(0, label_offset_y, str_info);

    if (control == STAR_CONTROL_BALL)
        rb->lcd_mono_bitmap (ball_bmp, 103, label_offset_y + 1, STAR_TILE_SIZE,
                             STAR_TILE_SIZE);
    else
        rb->lcd_mono_bitmap (block_bmp, 103, label_offset_y + 1, STAR_TILE_SIZE,
                             STAR_TILE_SIZE);

    rb->lcd_update_rect(0, label_offset_y, LCD_WIDTH, char_height);
}


/**
 * Load a level into board array.
 */
static int star_load_level(int current_level)
{
    int x, y;
    char *ptr_tab;

    ptr_tab = levels + current_level * STAR_LEVEL_SIZE;
    control = STAR_CONTROL_BALL;
    star_count = 0;

    rb->lcd_clear_display();

    for (y = 0 ; y < STAR_HEIGHT ; y++)
    {
        for (x = 0 ; x < STAR_WIDTH ; x++)
        {
            board[y][x] = *ptr_tab;
            switch (*ptr_tab)
            {
                case STAR_VOID:
                    break;

                case STAR_WALL:
                    rb->lcd_mono_bitmap (wall_bmp,
                                         STAR_OFFSET_X + x * STAR_TILE_SIZE,
                                         STAR_OFFSET_Y + y * STAR_TILE_SIZE,
                                         STAR_TILE_SIZE, STAR_TILE_SIZE);
                    break;

                case STAR_STAR:
                    rb->lcd_mono_bitmap (star_bmp,
                                         STAR_OFFSET_X + x * STAR_TILE_SIZE,
                                         STAR_OFFSET_Y + y * STAR_TILE_SIZE,
                                         STAR_TILE_SIZE, STAR_TILE_SIZE);
                    star_count++;
                    break;

                case STAR_BALL:
                    ball_x = x;
                    ball_y = y;
                    rb->lcd_mono_bitmap (ball_bmp,
                                         STAR_OFFSET_X + x * STAR_TILE_SIZE,
                                         STAR_OFFSET_Y + y * STAR_TILE_SIZE,
                                         STAR_TILE_SIZE, STAR_TILE_SIZE);
                    break;


                case STAR_BLOCK:
                    block_x = x;
                    block_y = y;
                    rb->lcd_mono_bitmap (block_bmp,
                                         STAR_OFFSET_X + x * STAR_TILE_SIZE,
                                         STAR_OFFSET_Y + y * STAR_TILE_SIZE,
                                         STAR_TILE_SIZE, STAR_TILE_SIZE);
                    break;
            }
            ptr_tab++;
        }
        ptr_tab++;
    }
    star_display_board_info();
    star_transition_update();
    return 1;
}

/**
 * Run the game.
 */
static int star_run_game(void)
{
    int move_x = 0;
    int move_y = 0;
    int i;
    int key;
    int lastkey = BUTTON_NONE;

    int label_offset_y;

    label_offset_y = LCD_HEIGHT - char_height;

    if (!star_load_level(current_level))
        return 0;

    while (true)
    {
        move_x = 0;
        move_y = 0;

        while ((move_x == 0) && (move_y == 0))
        {
            key = rb->button_get(true);
            switch (key)
            {
                case STAR_QUIT:
                    return 0;
                
                case BUTTON_LEFT:
                    move_x = -1;
                    break;

                case BUTTON_RIGHT:
                    move_x = 1;
                    break;

                case STAR_UP:
                    move_y = -1;
                    break;

                case STAR_DOWN:
                    move_y = 1;
                    break;

                case STAR_LEVEL_DOWN:
                    if (current_level > 0)
                    {
                        current_level--;
                        star_load_level(current_level);
                    }
                    break;

                case STAR_LEVEL_REPEAT:
                    star_load_level(current_level);
                    break;

                case STAR_LEVEL_UP:
                    if (current_level < STAR_LEVEL_COUNT - 1)
                    {
                        current_level++;
                        star_load_level(current_level);
                    }
                    break;

                case STAR_TOGGLE_CONTROL:
#ifdef STAR_TOGGLE_CONTROL_PRE
                    if (lastkey != STAR_TOGGLE_CONTROL_PRE)
                        break;
#endif
#ifdef STAR_TOGGLE_CONTROL2
                case STAR_TOGGLE_CONTROL2:
#endif
                    if (control == STAR_CONTROL_BALL)
                        control = STAR_CONTROL_BLOCK;
                    else
                        control = STAR_CONTROL_BALL;
                    star_display_board_info();
                    break;

                default:
                    if (rb->default_event_handler(key) == SYS_USB_CONNECTED)
                    {
                        usb_detected = true;
                        return 0;
                    }
                    break;
            }
            if (key != BUTTON_NONE)
                lastkey = key;
        }

        if (control == STAR_CONTROL_BALL)
        {
            board[ball_y][ball_x] = STAR_VOID;
            while ((board[ball_y + move_y][ball_x + move_x] == STAR_VOID
                    || board[ball_y + move_y][ball_x + move_x] == STAR_STAR))

            {
                for (i = 0 ; i < 7 ; i++)
                {
                    rb->lcd_mono_bitmap(
                        ball_bmp,
                        STAR_OFFSET_X + ball_x * STAR_TILE_SIZE + move_x * i,
                        STAR_OFFSET_Y + ball_y * STAR_TILE_SIZE + move_y * i,
                        STAR_TILE_SIZE, STAR_TILE_SIZE);

                    rb->lcd_update_rect(
                        STAR_OFFSET_X + ball_x * STAR_TILE_SIZE + move_x * i,
                        STAR_OFFSET_Y + ball_y * STAR_TILE_SIZE + move_y * i,
                        STAR_TILE_SIZE, STAR_TILE_SIZE);
                    rb->sleep(STAR_SLEEP);
                }
                ball_x += move_x;
                ball_y += move_y;

                if (board[ball_y][ball_x] == STAR_STAR)
                {
                    board[ball_y][ball_x] = STAR_VOID;
                    star_count--;

                    star_display_board_info();
                }
            }
            board[ball_y][ball_x] = STAR_BALL;
        }
        else
        {
            board[block_y][block_x] = STAR_VOID;
            while (board[block_y + move_y][block_x + move_x] == STAR_VOID)
            {
                for (i = 0 ; i < 7 ; i++)
                {
                    rb->lcd_mono_bitmap(
                        block_bmp,
                        STAR_OFFSET_X + block_x * STAR_TILE_SIZE + move_x * i,
                        STAR_OFFSET_Y + block_y * STAR_TILE_SIZE + move_y * i,
                        STAR_TILE_SIZE, STAR_TILE_SIZE);

                    rb->lcd_update_rect(
                        STAR_OFFSET_X + block_x * STAR_TILE_SIZE + move_x * i,
                        STAR_OFFSET_Y + block_y * STAR_TILE_SIZE + move_y * i,
                        STAR_TILE_SIZE, STAR_TILE_SIZE);

                    rb->sleep(STAR_SLEEP);
                }
                block_x += move_x;
                block_y += move_y;
            }
            board[block_y][block_x] = STAR_BLOCK;
        }

        if (star_count == 0)
        {
            current_level++;
            if (current_level == STAR_LEVEL_COUNT)
            {
                rb->lcd_clear_display();
                star_display_text("Congratulation !", true);
                rb->lcd_update();
                return 1;
            }
            star_load_level(current_level);
        }
    }
}

/**
 * Display the choice menu.
 */
static int star_menu(void)
{
    int move_y;
    int menu_y = 0;
    int i = 0;
    bool refresh = true;
    char anim_state = 0;
    unsigned char *menu[4] = {"Start", "Information", "Keys", "Exit"};
    int menu_count = sizeof(menu) / sizeof(unsigned char *);
    int menu_offset_y;
    int key;

    menu_offset_y = LCD_HEIGHT - char_height * menu_count;

    while (true)
    {
        if (refresh)
        {
            rb->lcd_clear_display();
            rb->lcd_putsxy((LCD_WIDTH - char_width *
                            rb->strlen(STAR_TITLE)) / 2,
                           0, STAR_TITLE);
            for (i = 0 ; i < menu_count ; i++)
            {
                rb->lcd_putsxy(15, menu_offset_y + char_height * i, menu[i]);
            }

            rb->lcd_update();
            refresh = false;
        }

        move_y = 0;
        rb->lcd_mono_bitmap(arrow_bmp[anim_arrow[(anim_state & 0x38) >> 3]],
                            2, menu_offset_y + menu_y * char_height, 7, 8);
        rb->lcd_update_rect (2, menu_offset_y + menu_y * 8, 8, 8);
        rb->sleep(STAR_SLEEP);
        anim_state++;

        key = rb->button_get(false);
        switch (key)
        {
            case STAR_QUIT:
                return PLUGIN_OK;
            case STAR_UP:
                if (menu_y > 0)
                    move_y = -1;
                break; 
            case STAR_DOWN:
                if (menu_y < 3)
                    move_y = 1;
                break; 

            case STAR_MENU_RUN:
#ifdef STAR_MENU_RUN3
            case STAR_MENU_RUN2:
            case STAR_MENU_RUN3:
#endif
                refresh = true;
                switch (menu_y)
                {
                    case 0:
                        if (!star_run_game())
                            return usb_detected ?
                                   PLUGIN_USB_CONNECTED : PLUGIN_OK;
                        break;
                    case 1:
                        star_display_text(
                            "INFO\n\n"
                            "Take all \"o\" to go to the next level. "
                            "You can toggle control with the block to "
                            "use it as a mobile wall. The block cannot "
                            "take \"o\".", true);
                        break;
                    case 2:
#if CONFIG_KEYPAD == RECORDER_PAD
                        star_display_text("KEYS\n\n"
                                          "[ON] Toggle Ctl.\n"
                                          "[OFF] Exit\n"
                                          "[F1] Prev. level\n"
                                          "[F2] Reset level\n"
                                          "[F3] Next level", true);
#elif CONFIG_KEYPAD == ONDIO_PAD
                        star_display_text("KEYS\n\n"
                                          "[MODE] Toggle Ctl\n"
                                          "[OFF] Exit\n"
                                          "[M <] Prev. level\n"
                                          "[M ^] Reset level\n"
                                          "[M >] Next level", true);
#endif
                        break;
                    case 3:
                        return PLUGIN_OK;
                }
                if (usb_detected)
                    return PLUGIN_USB_CONNECTED;
                break; 

            default:
                if (rb->default_event_handler(key) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }

        for (i = 0 ; i < char_height ; i++)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect (2, 30, 7, 4 * 8);
            rb->lcd_set_drawmode(DRMODE_FG);
            rb->lcd_mono_bitmap(arrow_bmp[anim_arrow[(anim_state & 0x38) >> 3]],
                                2, menu_offset_y + menu_y * 8 + move_y * i, 7, 8);
            rb->lcd_update_rect(2, 30, 8, 4 * 8);
            anim_state++;
            rb->sleep(STAR_SLEEP);
        }
            rb->lcd_set_drawmode(DRMODE_SOLID);
        menu_y += move_y;
    }
}

/**
 * Main entry point
 */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    (void)parameter;
    rb = api;

    /* get the size of char */
    rb->lcd_setfont(FONT_SYSFIXED);
    if (char_width == -1)
        rb->lcd_getstringsize("a", &char_width, &char_height);

    /* display choice menu */
    return star_menu();
}

#endif
