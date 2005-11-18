#include "yesno.h"
#include "kernel.h"
#include "misc.h"
#include "lang.h"

void gui_yesno_init(struct gui_yesno * yn,
                    struct text_message * main_message,
                    struct text_message * yes_message,
                    struct text_message * no_message)
{
    yn->main_message=main_message;
    yn->result_message[YESNO_YES]=yes_message;
    yn->result_message[YESNO_NO]=no_message;
    yn->display=0;
}

void gui_yesno_set_display(struct gui_yesno * yn,
                           struct screen * display)
{
    yn->display=display;
}

void gui_yesno_draw(struct gui_yesno * yn)
{
    struct screen * display=yn->display;
    int nb_lines, line_shift=0;
#ifdef HAS_LCD_BITMAP
    screen_set_xmargin(display, 0);
#endif
    gui_textarea_clear(display);
    nb_lines=yn->main_message->nb_lines;

    if(nb_lines+3<display->nb_lines)
        line_shift=1;
    nb_lines=gui_textarea_put_message(display, yn->main_message, line_shift);

    /* Space remaining for yes / no text ? */
    if(nb_lines+line_shift+2<=display->nb_lines)
    {
        if(nb_lines+line_shift+3<=display->nb_lines)
            nb_lines++;
        display->puts(0, nb_lines+line_shift, str(LANG_CONFIRM_WITH_PLAY_RECORDER));
        display->puts(0, nb_lines+line_shift+1, str(LANG_CANCEL_WITH_ANY_RECORDER));
    }
    gui_textarea_update(display);
}

bool gui_yesno_draw_result(struct gui_yesno * yn, enum yesno_res result)
{
    struct text_message * message=yn->result_message[result];
    if(message==NULL)
        return false;
    gui_textarea_put_message(yn->display, message, 0);
    return(true);
}

enum yesno_res gui_syncyesno_run(struct text_message * main_message,
                             struct text_message * yes_message,
                             struct text_message * no_message)
{
    int i;
    unsigned button;
    int result=-1;
    bool result_displayed;
    struct gui_yesno yn[NB_SCREENS];
    FOR_NB_SCREENS(i)
    {
        gui_yesno_init(&(yn[i]), main_message, yes_message, no_message);
        gui_yesno_set_display(&(yn[i]), &(screens[i]));
        gui_yesno_draw(&(yn[i]));
    }
    while (result==-1)
    {
        button = button_get(true);
        switch (button)
        {
            case YESNO_OK:
#ifdef TREE_RC_RUN
            case YESNO_RC_OK:
#endif
                result=YESNO_YES;
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    return(YESNO_USB);
                if(!(button & BUTTON_REL))
                    result=YESNO_NO;
        }
    }
    FOR_NB_SCREENS(i)
        result_displayed=gui_yesno_draw_result(&(yn[i]), result);
    if(result_displayed)
        sleep(HZ);
    return(result);
}
