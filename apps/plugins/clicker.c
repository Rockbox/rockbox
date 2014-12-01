#include "plugin.h"
#include "lib/pluginlib_actions.h"


plugin_start(const void* param){
    (void)param;
    const struct button_mapping *ctxs[]={pla_main_ctx};
    int run=1;
    int n=0;
    int f=0;

    rb->lcd_clear_display ();
    rb->lcd_update();

    while (run == 1) {
        switch (pluginlib_getaction(-1,ctxs,1))
        {
            case PLA_SELECT:
                if (f != 1) {
                    ++n;
                    rb->lcd_putsf(0,0,"%d",n);
                    rb->lcd_update();
                    f=1;
                }
                break;
            case PLA_EXIT:
                run=0;
                break;
            default :
                f=0;
                break;
        }
    }
    return PLUGIN_OK;
}
