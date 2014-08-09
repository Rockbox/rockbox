#include "plugin.h"
#include "lib/pluginlib_actions.h"
#include "lib/pluginlib_exit.h"
const struct button_mapping *pla_context[]={pla_main_ctx};
enum plugin_status plugin_start(const void* param)
{
    (void)param;
    rb->splash(HZ, "Open 6th");
    rb->piezo_play(1000000, 82, true);
    rb->splash(HZ, "Middle C");
    rb->piezo_play(1000000, 262, true);
    rb->splash(HZ, "Beep");
    rb->piezo_beep(true);
    rb->splash(HZ, "Click");
    rb->piezo_click(true);
    rb->splash(HZ*2, "Use up/down to select frequency");
    int freq=200;
    for(;;)
    {
        rb->piezo_play(1000000, freq, false);
        char buf[32];
        rb->snprintf(buf, 32, "%d", freq);
        rb->lcd_putsxy(0,0,buf);
        rb->lcd_update();
        int action=pluginlib_getaction(-1, pla_context, 1);
        switch(action)
        {
        case PLA_UP:
        case PLA_UP_REPEAT:
            freq+=10;
            break;
        case PLA_DOWN:
        case PLA_DOWN_REPEAT:
            freq-=10;
            break;
        case PLA_CANCEL:
            return PLUGIN_OK;
        default:
            exit_on_usb(action);
            break;
        }
    }
    return PLUGIN_OK;
}
