#include "zxconfig.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

#if CONFIG_KEYPAD == RECORDER_PAD
#define BUTTONBAR_HEIGHT 8
#else
#define BUTTONBAR_HEIGHT 0
#endif

#define DEFAULT_MARGIN 6
#define KBD_BUF_SIZE 500
#define kbd_loaded false
#define statusbar_size 0

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define KBD_SELECT BUTTON_SELECT
#define KBD_ABORT BUTTON_OFF
#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_UP
#define KBD_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == RECORDER_PAD
#define KBD_SELECT BUTTON_PLAY
#define KBD_ABORT BUTTON_OFF
#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_UP
#define KBD_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define KBD_SELECT BUTTON_SELECT
#define KBD_ABORT BUTTON_OFF
#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_UP
#define KBD_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == ONDIO_PAD /* restricted Ondio keypad */
#define KBD_SELECT BUTTON_MENU
#define KBD_ABORT BUTTON_OFF
#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_UP
#define KBD_DOWN BUTTON_DOWN

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)

#define KBD_SELECT BUTTON_SELECT
#define KBD_ABORT BUTTON_MENU
#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_SCROLL_BACK
#define KBD_DOWN BUTTON_SCROLL_FWD

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD

/* TODO: Check keyboard mappings */

#define KBD_SELECT BUTTON_SELECT 
#define KBD_ABORT BUTTON_PLAY
#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_UP
#define KBD_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD

/* TODO: Check keyboard mappings */

#define KBD_SELECT BUTTON_SELECT
#define KBD_ABORT BUTTON_REC
#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_UP
#define KBD_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == GIGABEAT_PAD

#define KBD_SELECT BUTTON_SELECT
#define KBD_ABORT BUTTON_POWER
#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_UP
#define KBD_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD

#define KBD_SELECT BUTTON_SELECT
#define KBD_ABORT BUTTON_POWER
#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_UP
#define KBD_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == IRIVER_H10_PAD

/* TODO: Check keyboard mappings */

#define KBD_SELECT BUTTON_REW
#define KBD_ABORT BUTTON_FF
#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_SCROLL_UP
#define KBD_DOWN BUTTON_SCROLL_DOWN

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
(CONFIG_KEYPAD == SANSA_C200_PAD) || \
(CONFIG_KEYPAD == MROBE100_PAD)

/* TODO: Check keyboard mappings */

#define KBD_SELECT BUTTON_SELECT
#define KBD_ABORT BUTTON_POWER
#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_UP
#define KBD_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD

#define KBD_SELECT BUTTON_RC_PLAY
#define KBD_ABORT BUTTON_RC_REC
#define KBD_LEFT BUTTON_RC_REW
#define KBD_RIGHT BUTTON_RC_FF
#define KBD_UP BUTTON_RC_VOL_UP
#define KBD_DOWN BUTTON_RC_VOL_DOWN

#elif CONFIG_KEYPAD == COWOND2_PAD

#define KBD_ABORT BUTTON_POWER

#elif CONFIG_KEYPAD == IAUDIO67_PAD

#define KBD_SELECT BUTTON_MENU
#define KBD_ABORT BUTTON_POWER
#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_STOP
#define KBD_DOWN BUTTON_PLAY
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef KBD_SELECT
#define KBD_SELECT BUTTON_CENTER
#endif
#ifndef KBD_ABORT
#define KBD_ABORT  BUTTON_TOPLEFT
#endif
#ifndef KBD_LEFT
#define KBD_LEFT   BUTTON_MIDLEFT
#endif
#ifndef KBD_RIGHT
#define KBD_RIGHT  BUTTON_MIDRIGHT
#endif
#ifndef KBD_UP
#define KBD_UP     BUTTON_TOPMIDDLE
#endif
#ifndef KBD_DOWN
#define KBD_DOWN   BUTTON_BOTTOMMIDDLE
#endif
#endif

struct keyboard_parameters {
    const unsigned char* default_kbd;
    int DEFAULT_LINES;
    unsigned short kbd_buf[KBD_BUF_SIZE];
    int nchars;
    int font_w;
    int font_h;
    struct font* font;
    int curfont;
    int main_x;
    int main_y;
    int max_chars;
    int max_chars_text;
    int lines;
    int pages;
    int keyboard_margin;
    int old_main_y;
    int curpos;
    int leftpos;
    int page;
    int x;
    int y;
};

struct keyboard_parameters param[NB_SCREENS];

int zx_kbd_input(char* text/*, int buflen*/)
{
    bool done = false;
    int i, j, k, w, l;
    int text_w = 0;
    int len_utf8/*, c = 0*/;
    int editpos;
/*    int statusbar_size = global_settings.statusbar ? STATUSBAR_HEIGHT : 0;*/
    unsigned short ch/*, tmp, hlead = 0, hvowel = 0, htail = 0*/;
    /*bool hangul = false;*/
    unsigned char *utf8;
    const unsigned char *p;
    bool cur_blink = true;
    int char_screen = 0;

    FOR_NB_SCREENS(l)
    {
             param[l].default_kbd =
                      "1234567890\n"
                      "qwertyuiop\n"
                      "asdfghjkl\n"
                      "zxcvbnm\n"
                      "S\n"
                      "E";

             param[l].DEFAULT_LINES = 7;
    }

    char outline[256];
    int button, lastbutton = 0;
    FOR_NB_SCREENS(l)
    {
            /* Copy default keyboard to buffer */
            i = 0;
            param[l].curfont = FONT_SYSFIXED;
            p = param[l].default_kbd;
            while (*p != 0)
                p = rb->utf8decode(p, &param[l].kbd_buf[i++]);
            param[l].nchars = i;
/*            param[l].curfont = FONT_UI;*/
    }
    FOR_NB_SCREENS(l)
    {
        param[l].font = rb->font_get(param[l].curfont);
        param[l].font_h = param[l].font->height;

            /* check if FONT_UI fits the screen */
        if (2*param[l].font_h+3 + BUTTONBAR_HEIGHT >
            rb->screens[l]->getheight()) {
            param[l].font = rb->font_get(FONT_SYSFIXED);
            param[l].font_h = param[l].font->height;
            param[l].curfont = FONT_SYSFIXED;
        }

        rb->screens[l]->setfont(param[l].curfont);
        /* find max width of keyboard glyphs */
        for (i=0; i<param[l].nchars; i++) {
            w = rb->font_get_width(param[l].font, param[l].kbd_buf[i]);
            if (w > param[l].font_w)
                param[l].font_w = w;
        }
        /* Since we're going to be adding spaces, make sure that we check
         * their width too */
        w = rb->font_get_width( param[l].font, ' ' );
        if( w > param[l].font_w )
            param[l].font_w = w;
    }
    FOR_NB_SCREENS(l)
    {
        i = 0;
        /* Pad lines with spaces */
        while( i < param[l].nchars )
        {
            if( param[l].kbd_buf[i] == '\n' )
            {
                k = ( rb->screens[l]->getwidth() / param[l].font_w ) -
                    i % ( rb->screens[l]->getwidth() / param[l].font_w ) - 1;
                if( k == ( rb->screens[l]->getwidth() / param[l].font_w ) - 1 )
                {
                    param[l].nchars--;
                    for( j = i; j < param[l].nchars; j++ )
                    {
                        param[l].kbd_buf[j] = param[l].kbd_buf[j+1];
                    }
                }
                else
                {
                    if( param[l].nchars + k - 1 >= KBD_BUF_SIZE )
                    {   /* We don't want to overflow the buffer */
                        k = KBD_BUF_SIZE - param[l].nchars;
                    }
                    for( j = param[l].nchars + k - 1; j > i+k; j-- )
                    {
                        param[l].kbd_buf[j] = param[l].kbd_buf[j-k];
                    }
                    param[l].nchars += k;
                    k++;
                    while( k-- )
                    {
                        param[l].kbd_buf[i++] = ' ';
                    }
                }
            }
            else
                i++;
        }
    }

    /* find max width for text string */
    utf8 = text;
    FOR_NB_SCREENS(l)
    {
        text_w = param[l].font_w;
        while (*utf8) {
            utf8 = (unsigned char*)rb->utf8decode(utf8, &ch);
            w = rb->font_get_width(param[l].font, ch);
            if (w > text_w)
                text_w = w;
        }
        param[l].max_chars_text = rb->screens[l]->getwidth() / text_w - 2;

    /* calculate keyboard grid size */
        param[l].max_chars = rb->screens[l]->getwidth() / param[l].font_w;
        if (!kbd_loaded) {
            param[l].lines = param[l].DEFAULT_LINES;
            param[l].keyboard_margin = DEFAULT_MARGIN;
        } else {
            param[l].lines = (rb->screens[l]->lcdheight - BUTTONBAR_HEIGHT -
                             statusbar_size) / param[l].font_h - 1;
            param[l].keyboard_margin = rb->screens[l]->lcdheight -
                                       BUTTONBAR_HEIGHT - statusbar_size -
                                       (param[l].lines+1)*param[l].font_h;
            if (param[l].keyboard_margin < 3) {
                param[l].lines--;
                param[l].keyboard_margin += param[l].font_h;
            }
            if (param[l].keyboard_margin > 6)
                param[l].keyboard_margin = 6;
        }

        param[l].pages = (param[l].nchars + (param[l].lines*param[l].max_chars-1))
             /(param[l].lines*param[l].max_chars);
        if (param[l].pages == 1 && kbd_loaded)
            param[l].lines = (param[l].nchars + param[l].max_chars - 1) / param[l].max_chars;

        param[l].main_y = param[l].font_h*param[l].lines + param[l].keyboard_margin + statusbar_size;
        param[l].main_x = 0;
        param[l].keyboard_margin -= param[l].keyboard_margin/2;

    }
    editpos = rb->utf8length(text);



    while(!done)
    {
        len_utf8 = rb->utf8length(text);
        FOR_NB_SCREENS(l)
            rb->screens[l]->clear_display();


            /* draw page */
            FOR_NB_SCREENS(l)
            {
                rb->screens[l]->setfont(param[l].curfont);
                k = param[l].page*param[l].max_chars*param[l].lines;
                for (i=j=0; j < param[l].lines && k < param[l].nchars; k++) {
                    utf8 = rb->utf8encode(param[l].kbd_buf[k], outline);
                    *utf8 = 0;
                    rb->screens[l]->getstringsize(outline, &w, NULL);
                    rb->screens[l]->putsxy(i*param[l].font_w + (param[l].font_w-w)/2, j*param[l].font_h
                          + statusbar_size, outline);
                    if (++i == param[l].max_chars) {
                        i = 0;
                        j++;
                    }
                }
            }


        /* separator */
        FOR_NB_SCREENS(l)
        {
            rb->screens[l]->hline(0, rb->screens[l]->getwidth() - 1,
                                  param[l].main_y - param[l].keyboard_margin);

        /* write out the text */
#if 0
			rb->screens[l]->setfont(param[l].curfont);

            i=j=0;
            param[l].curpos = MIN(editpos, param[l].max_chars_text
                                - MIN(len_utf8 - editpos, 2));
            param[l].leftpos = editpos - param[l].curpos;
            utf8 = text + rb->utf8seek(text, param[l].leftpos);

            text_w = param[l].font_w;
            while (*utf8 && i < param[l].max_chars_text) {
                outline[j++] = *utf8++;
                if ((*utf8 & MASK) != COMP) {
                    outline[j] = 0;
                    j=0;
                    i++;
                    rb->screens[l]->getstringsize(outline, &w, NULL);
                    rb->screens[l]->putsxy(i*text_w + (text_w-w)/2, param[l].main_y, outline);
                }
            }

            if (param[l].leftpos) {
                rb->screens[l]->getstringsize("<", &w, NULL);
                rb->screens[l]->putsxy(text_w - w, param[l].main_y, "<");
            }
            if (len_utf8 - param[l].leftpos > param[l].max_chars_text)
                rb->screens[l]->putsxy(rb->screens[l]->width - text_w, param[l].main_y, ">");

            /* cursor */
            i = (param[l].curpos + 1) * text_w;
            if (cur_blink)
                rb->screens[l]->vline(i, param[l].main_y, param[l].main_y + param[l].font_h-1);

            if (hangul) /* draw underbar */
                rb->screens[l]->hline(param[l].curpos*text_w, (param[l].curpos+1)*text_w,
                                       param[l].main_y+param[l].font_h-1);
#endif
		}
        cur_blink = !cur_blink;

        
            /* highlight the key that has focus */
            FOR_NB_SCREENS(l)
            {
                rb->screens[l]->set_drawmode(DRMODE_COMPLEMENT);
                rb->screens[l]->fillrect(param[l].font_w * param[l].x,
                                        statusbar_size + param[l].font_h * param[l].y,
                                        param[l].font_w, param[l].font_h);
                rb->screens[l]->set_drawmode(DRMODE_SOLID);
            }
        

/*        gui_syncstatusbar_draw(&statusbars, true);*/
        FOR_NB_SCREENS(l)
        rb->screens[l]->update();

        button = rb->button_get_w_tmo(HZ/2);

        switch ( button ) {

            case KBD_ABORT:
                FOR_NB_SCREENS(l)
                    rb->screens[l]->setfont(FONT_UI);

                return -1;
                break;

            case KBD_RIGHT:
            case KBD_RIGHT | BUTTON_REPEAT:
                {
                    FOR_NB_SCREENS(l)
                    {
                        if (++param[l].x == param[l].max_chars) {
                            param[l].x = 0;
                        /* no dedicated flip key - flip page on wrap */
                            if (++param[l].page == param[l].pages)
                                param[l].page = 0;
                        }
                        k = (param[l].page*param[l].lines + param[l].y)*param[l].max_chars + param[l].x;
                        /*kbd_spellchar(param[l].kbd_buf[k]);*/
                    }
                }
                break;
            case KBD_LEFT:
            case KBD_LEFT | BUTTON_REPEAT:
                {
                    FOR_NB_SCREENS(l)
                    {
                        if (param[l].x)
                            param[l].x--;
                        else
                        {
                        /* no dedicated flip key - flip page on wrap */
                            if (--param[l].page < 0)
                                param[l].page = (param[l].pages-1);
                            param[l].x = param[l].max_chars - 1;
                        }
                        k = (param[l].page*param[l].lines +
                            param[l].y)*param[l].max_chars + param[l].x;
                       /* kbd_spellchar(param[l].kbd_buf[k]);*/
                    }
                }
                break;

            case KBD_DOWN:
            case KBD_DOWN | BUTTON_REPEAT:
                {
                    FOR_NB_SCREENS(l)
                    {
                        if (param[l].y < param[l].lines - 1)
                            param[l].y++;
                        else
                            param[l].y=0;
                    }
                    FOR_NB_SCREENS(l)
                    {
                        k = (param[l].page*param[l].lines + param[l].y)*
                                 param[l].max_chars + param[l].x;
                        /*kbd_spellchar(param[l].kbd_buf[k]);*/
                    }
                }
                break;

            case KBD_UP:
            case KBD_UP | BUTTON_REPEAT:
                {
                    FOR_NB_SCREENS(l)
                    {
                        if (param[l].y)
                            param[l].y--;
                        else
                            param[l].y = param[l].lines - 1;
                    }
                    FOR_NB_SCREENS(l)
                    {
                        k = (param[l].page*param[l].lines + param[l].y)*
                               param[l].max_chars + param[l].x;
                        /*kbd_spellchar(param[l].kbd_buf[k]);*/
                    }
                }
                break;

            case KBD_SELECT:
            {
                if (button == KBD_SELECT)
                    char_screen = 0;

                /* inserts the selected char */
                
                    /* find input char */
                        k = (param[char_screen].page*param[char_screen].lines +
                            param[char_screen].y)*param[char_screen].max_chars +
                            param[char_screen].x;
                        if (k < param[char_screen].nchars)
                            ch = param[char_screen].kbd_buf[k];
                        else
                            ch = ' ';
                        text[0]=ch;
                        done = true;
            }
                break;

            case BUTTON_NONE:
                /*gui_syncstatusbar_draw(&statusbars, false);*/

                break;

            default:
                if(rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    FOR_NB_SCREENS(l)
                        rb->screens[l]->setfont(FONT_SYSFIXED);
                break;

        }
        if (button != BUTTON_NONE)
        {
            lastbutton = button;
            cur_blink = true;
        }
    }
    FOR_NB_SCREENS(l)
        rb->screens[l]->setfont(FONT_UI);
    return 0;
}
