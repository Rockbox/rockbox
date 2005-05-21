/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: flipit.c,v 1.0 2003/01/18 23:51:47
 *
 * Copyright (C) 2002 Vicentini Martin
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

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define FLIPIT_QUIT BUTTON_OFF
#define FLIPIT_SHUFFLE BUTTON_F1
#define FLIPIT_SOLVE BUTTON_F2
#define FLIPIT_STEP_BY_STEP BUTTON_F3
#define FLIPIT_TOGGLE BUTTON_PLAY

#elif CONFIG_KEYPAD == ONDIO_PAD
#define FLIPIT_QUIT BUTTON_OFF
#define FLIPIT_SHUFFLE (BUTTON_MENU | BUTTON_LEFT)
#define FLIPIT_SOLVE (BUTTON_MENU | BUTTON_UP)
#define FLIPIT_STEP_BY_STEP (BUTTON_MENU | BUTTON_RIGHT)
#define FLIPIT_TOGGLE_PRE BUTTON_MENU
#define FLIPIT_TOGGLE (BUTTON_MENU | BUTTON_REL)

#elif CONFIG_KEYPAD == IRIVER_H100_PAD
#define FLIPIT_QUIT BUTTON_OFF
#define FLIPIT_SHUFFLE BUTTON_MODE
#define FLIPIT_SOLVE BUTTON_ON
#define FLIPIT_STEP_BY_STEP BUTTON_REC
#define FLIPIT_TOGGLE_PRE BUTTON_SELECT
#define FLIPIT_TOGGLE (BUTTON_SELECT | BUTTON_REL)

#endif

static struct plugin_api* rb;
static int spots[20];
static int toggle[20];
static int cursor_pos, moves;
static char s[5];
static char *ptr;

#if LCD_WIDTH == 160
#define SPOT_SIZE 20
#define SPOT_SPACE 4
#define MARGIN_TOP 16
#define MARGIN_LEFT 5
static unsigned char spot_pic[2][60] = {
    { 0xe0, 0xf8, 0xfc, 0xfe, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xfe, 0xfc, 0xf8, 0xe0,
      0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f,
      0x00, 0x01, 0x03, 0x07, 0x07, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
      0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x07, 0x07, 0x03, 0x01, 0x00, },
    { 0xe0, 0x18, 0x0c, 0x06, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01,
      0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x06, 0x0c, 0x18, 0xe0,
      0x7f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x7f,
      0x00, 0x01, 0x03, 0x06, 0x04, 0x08, 0x08, 0x08, 0x08, 0x08,
      0x08, 0x08, 0x08, 0x08, 0x08, 0x04, 0x06, 0x03, 0x01, 0x00, }
};

#define CURSOR_SIZE 22
static unsigned char cursor_pic[66] = {
      0x55, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
      0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
      0x01, 0xaa, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0xaa, 0x15, 0x20, 0x00, 0x20, 0x00, 0x20,
      0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
      0x00, 0x20, 0x00, 0x20, 0x00, 0x2a,
};

#else
#define SPOT_SIZE 14
#define SPOT_SPACE 2
#define MARGIN_TOP 0
#define MARGIN_LEFT 0
static unsigned char spot_pic[2][28] = {
    { 0xe0, 0xf8, 0xfc, 0xfe, 0xfe, 0xff, 0xff,
      0xff, 0xff, 0xfe, 0xfe, 0xfc, 0xf8, 0xe0,
      0x01, 0x07, 0x0f, 0x1f, 0x1f, 0x3f, 0x3f,
      0x3f, 0x3f, 0x1f, 0x1f, 0x0f, 0x07, 0x01 },
    { 0xe0, 0x18, 0x04, 0x02, 0x02, 0x01, 0x01,
      0x01, 0x01, 0x02, 0x02, 0x04, 0x18, 0xe0,
      0x01, 0x06, 0x08, 0x10, 0x10, 0x20, 0x20,
      0x20, 0x20, 0x10, 0x10, 0x08, 0x06, 0x01 }
};

#define CURSOR_SIZE 16
static unsigned char cursor_pic[32] = {
    0x55, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0xaa,
    0x55, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80,
    0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0xaa };
#endif

/* draw a spot at the coordinates (x,y), range of p is 0-19 */
static void draw_spot(int p) {
    ptr = spot_pic[spots[p]];
    int x,y;
    x = (p%5)*(SPOT_SIZE + SPOT_SPACE)+1 + MARGIN_LEFT;
    y = (p/5)*(SPOT_SIZE + SPOT_SPACE)+1 + MARGIN_TOP;
    rb->lcd_bitmap (ptr, x, y, SPOT_SIZE, SPOT_SIZE, true);
/*
    rb->lcd_bitmap (ptr, (p%5)*16+1, (p/5)*16+1, 14, 8, true);
    ptr += 14;
    rb->lcd_bitmap (ptr, (p%5)*16+1, (p/5)*16+9, 14, 6, true);
*/
}

/* draw the cursor at the current cursor position */
static void draw_cursor(void) {
    int i,j;
    i = (cursor_pos%5)*CURSOR_SIZE + (SPOT_SPACE/2)*(cursor_pos%5) + MARGIN_LEFT;
    j = (cursor_pos/5)*CURSOR_SIZE + (SPOT_SPACE/2)*(cursor_pos/5) + MARGIN_TOP;
    ptr = cursor_pic;
    rb->lcd_bitmap (ptr, i, j, CURSOR_SIZE, CURSOR_SIZE, false);
}

/* clear the cursor where it is */
static void clear_cursor(void) {
    int i,j;
    i = (cursor_pos%5)*CURSOR_SIZE + (SPOT_SPACE/2)*(cursor_pos%5) + MARGIN_LEFT;
    j = (cursor_pos/5)*CURSOR_SIZE + (SPOT_SPACE/2)*(cursor_pos/5) + MARGIN_TOP;
    rb->lcd_clearline(i, j, i+CURSOR_SIZE-1, j);
    rb->lcd_clearline(i, j+CURSOR_SIZE-1, i+CURSOR_SIZE-1, j+CURSOR_SIZE-1);
    rb->lcd_clearline(i, j, i, j+CURSOR_SIZE-1);
    rb->lcd_clearline(i+CURSOR_SIZE-1, j, i+CURSOR_SIZE-1, j+CURSOR_SIZE-1);
}

/* check if the puzzle is finished */
static bool flipit_finished(void) {
    int i;
    for (i=0; i<20; i++)
    if (!spots[i])
        return false;
    clear_cursor();
    return true;
}

/* draws the toggled spots */
static void flipit_toggle(void) {
    spots[cursor_pos] = 1-spots[cursor_pos];
    toggle[cursor_pos] = 1-toggle[cursor_pos];
    draw_spot(cursor_pos);
    if (cursor_pos%5 > 0) {
        spots[cursor_pos-1] = 1-spots[cursor_pos-1];
        draw_spot(cursor_pos-1);
    }
    if (cursor_pos%5 < 4) {
        spots[cursor_pos+1] = 1-spots[cursor_pos+1];
        draw_spot(cursor_pos+1);
    }
    if (cursor_pos/5 > 0) {
        spots[cursor_pos-5] = 1-spots[cursor_pos-5];
        draw_spot(cursor_pos-5);
    }
    if (cursor_pos/5 < 3) {
        spots[cursor_pos+5] = 1-spots[cursor_pos+5];
        draw_spot(cursor_pos+5);
    }
    moves++;
    rb->snprintf(s, sizeof(s), "%d", moves);
    rb->lcd_putsxy(LCD_WIDTH - 27, 20, s);
    if (flipit_finished())
        clear_cursor();
}

/* move the cursor in any direction */
static void move_cursor(int x, int y) {
    if (!(flipit_finished())) {
        clear_cursor();
        cursor_pos += (x+5*y);
        draw_cursor();
    }
    rb->lcd_update();
}

/* initialize the board */
static void flipit_init(void) {
    int i;
    rb->lcd_clear_display();
    moves = 0;
    rb->lcd_drawrect(LCD_WIDTH - 32, 0, 32, LCD_HEIGHT);
    rb->lcd_putsxy(LCD_WIDTH - 31, 10, "Flips");
    for (i=0; i<20; i++) {
        spots[i]=1;
        toggle[i]=1;
        draw_spot(i);
    }
    rb->lcd_update();
    for (i=0; i<20; i++) {
        cursor_pos = (rb->rand() % 20);
        flipit_toggle();
    }

    cursor_pos = 0;
    draw_cursor();
    moves = 0;
    rb->lcd_clearrect(LCD_WIDTH - 32, 0, 32, LCD_HEIGHT);
    rb->lcd_drawrect(LCD_WIDTH - 32, 0, 32, LCD_HEIGHT);
    rb->lcd_putsxy(LCD_WIDTH - 31, 10, "Flips");
    rb->snprintf(s, sizeof(s), "%d", moves);
    rb->lcd_putsxy(LCD_WIDTH - 27, 20, s);
    rb->lcd_update();
}

/* the main game loop */
static bool flipit_loop(void) {
    int i;
    int button;
    int lastbutton = BUTTON_NONE;

    flipit_init();
    while(true) {
        button = rb->button_get(true);
        switch (button) {
            case FLIPIT_QUIT:
                /* get out of here */
                return PLUGIN_OK;
    
            case FLIPIT_SHUFFLE:
                /* mix up the pieces */
                flipit_init();
                break;

            case FLIPIT_SOLVE:
                /* solve the puzzle */
                if (!flipit_finished()) {
                    for (i=0; i<20; i++)
                        if (!toggle[i]) {
                            clear_cursor();
                            cursor_pos = i;
                            draw_cursor();
                            flipit_toggle();
                            rb->lcd_update();
                            rb->sleep(HZ*2/3);
                        }
                }
                break;

            case FLIPIT_STEP_BY_STEP:
                if (!flipit_finished()) {
                    for (i=0; i<20; i++)
                        if (!toggle[i]) {
                            clear_cursor();
                            cursor_pos = i;
                            draw_cursor();
                            flipit_toggle();
                            rb->lcd_update();
                            break;
                        }
                }
                break;

            case FLIPIT_TOGGLE:
#ifdef FLIPIT_TOGGLE_PRE
                if (lastbutton != FLIPIT_TOGGLE_PRE)
                    break;
#endif
                /* toggle the pieces */
                if (!flipit_finished()) {
                    flipit_toggle();
                    rb->lcd_update();
                }
                break;

            case BUTTON_LEFT:
                if ((cursor_pos%5)>0)
                    move_cursor(-1, 0);
                break;

            case BUTTON_RIGHT:
                if ((cursor_pos%5)<4)
                    move_cursor(1, 0);
                break;

            case BUTTON_UP:
                if ((cursor_pos/5)>0)
                    move_cursor(0, -1);
                break;

            case BUTTON_DOWN:
                if ((cursor_pos/5)<3)
                    move_cursor(0, 1);
                break;

            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
        if (button != BUTTON_NONE)
            lastbutton = button;
    }
}

/* called function from outside */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int w, h, i;

    TEST_PLUGIN_API(api);
    (void)parameter;
    rb = api;

    /* print title */
    rb->lcd_getstringsize("FlipIt!", &w, &h);
    w = (w+1)/2;
    h = (h+1)/2;
    rb->lcd_clear_display();
    rb->lcd_putsxy(LCD_WIDTH/2-w, (LCD_HEIGHT/2)-h, "FlipIt!");
    rb->lcd_update();
    rb->sleep(HZ);

    /* print instructions */
    rb->lcd_clear_display();
    rb->lcd_setfont(FONT_SYSFIXED);
#if CONFIG_KEYPAD == RECORDER_PAD
    rb->lcd_putsxy(2, 8, "[OFF] to stop");
    rb->lcd_putsxy(2, 18, "[PLAY] toggle");
    rb->lcd_putsxy(2, 28, "[F1] shuffle");
    rb->lcd_putsxy(2, 38, "[F2] solution");
    rb->lcd_putsxy(2, 48, "[F3] step by step");
#elif CONFIG_KEYPAD == ONDIO_PAD
    rb->lcd_putsxy(2, 8, "[OFF] to stop");
    rb->lcd_putsxy(2, 18, "[MODE] toggle");
    rb->lcd_putsxy(2, 28, "[M-LEFT] shuffle");
    rb->lcd_putsxy(2, 38, "[M-UP] solution");
    rb->lcd_putsxy(2, 48, "[M-RIGHT] step by step");
#elif CONFIG_KEYPAD == IRIVER_H100_PAD
    rb->lcd_putsxy(2, 8, "[STOP] to stop");
    rb->lcd_putsxy(2, 18, "[SELECT] toggle");
    rb->lcd_putsxy(2, 28, "[MODE] shuffle");
    rb->lcd_putsxy(2, 38, "[PLAY] solution");
    rb->lcd_putsxy(2, 48, "[REC] step by step");
#endif
    rb->lcd_update();
    rb->sleep(HZ*3);

    rb->lcd_clear_display();
    rb->lcd_drawrect(LCD_WIDTH - 32, 0, 32, LCD_HEIGHT);
    rb->lcd_putsxy(LCD_WIDTH - 31, 10, "Flips");
    for (i=0; i<20; i++) {
        spots[i]=1;
        draw_spot(i);
    }
    rb->lcd_update();
    rb->sleep(HZ*3/2);
    rb->srand(*rb->current_tick);

    return flipit_loop();
}
#endif
