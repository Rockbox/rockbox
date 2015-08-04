#include "plugin.h"
#include "lib/pluginlib_actions.h"
#include "lib/pluginlib_exit.h"

static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };

void myread_line(int fd, char *buf, size_t sz)
{
    if(rb->read_line(fd, buf, sz) < 0)
    {
        rb->close(fd);
        exit(PLUGIN_OK);
    }
}

enum plugin_status plugin_start(const void* param)
{
    (void) param;

    int fd = rb->open("/data", O_RDONLY);

    rb->lcd_setfont(rb->font_load("/.rockbox/fonts/07-Fixed.fnt"));

    while(1)
    {
        rb->lcd_clear_display();

        char delaybuf[8] = { 0 };
        myread_line(fd, delaybuf, sizeof(delaybuf));

        unsigned frametime = strtoul(delaybuf, NULL, 10);

        for(int i = 0; i < 13; ++i)
        {
            char linebuf[80];
            myread_line(fd, linebuf, sizeof(linebuf));
            rb->lcd_puts(0, i, linebuf);
        }

        rb->lcd_update();
        for(unsigned i = 0; i < frametime; ++i)
        {
            /* one timeslice = 1/15 second */
            const int slice_ticks = 1/15.0 * HZ;
            rb->sleep(slice_ticks);
        }
        int button = pluginlib_getaction(0, plugin_contexts, ARRAYLEN(plugin_contexts));
        switch(button)
        {
        case PLA_EXIT:
            return PLUGIN_OK;
        default:
            exit_on_usb(button);
        }

        rb->yield();
    }
}
