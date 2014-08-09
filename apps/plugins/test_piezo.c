#include "plugin.h"
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
    return PLUGIN_OK;
}
