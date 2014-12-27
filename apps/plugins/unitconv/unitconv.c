#include "plugin.h"
#include "lib/pluginlib_actions.h"
#include <stdbool.h>
/* data headers go below */
#include "mass.h"

static void convert_units(struct unit_t *convtab, const size_t len, int idx)
{
    /* get the value of the current unit in terms of the 0th unit in the data table */
    uint64_t val = convtab[idx].value / convtab[0].ratio;
    for(int i=0;i<len;++i)
    {
        if(i != idx)
        {
            convtab[i].value = val * convtab[i].ratio;
        }
    }
}

static void draw_everything(struct unit_t *convtab, const size_t len, int idx)
{
    convert_units(convtab, len, idx);
    /* TODO */
    rb->lcd_clear_display();
    for(int i=0;i<len;++i)
    {
        if(i == idx)
        {
            rb->lcd_set_foreground(LCD_BLACK);
            rb->lcd_set_background(LCD_WHITE);
        }
        else
        {
            rb->lcd_set_foreground(LCD_WHITE);
            rb->lcd_set_background(LCD_BLACK);
        }
        rb->lcd_putsf(0, i, "%llu.%llu %s", convtab[i].value/convtab[0].ratio, convtab[i].value%convtab[0].ratio, convtab[i].abbr);
    }
    rb->lcd_update();
}

static void do_unitconv(struct unit_t *convtab, const size_t len)
{
    const struct button_mapping *plugin_contexts[] = { pla_main_ctx };
    int idx = 0;
    bool quit = false;
    while(!quit)
    {
        switch(pluginlib_getaction(-1, plugin_contexts, ARRAYLEN(plugin_contexts)))
        {
        case PLA_DOWN:
        case PLA_DOWN_REPEAT:
            if(idx++ >= len)
                idx=0;
            break;
        case PLA_UP:
        case PLA_UP_REPEAT:
            if(idx-- < 0)
                idx = len-1;
            break;
        case PLA_RIGHT:
        case PLA_RIGHT_REPEAT:
            convtab[idx].value += UNIT;
            break;
        case PLA_LEFT:
        case PLA_LEFT_REPEAT:
            convtab[idx].value -= UNIT;
            break;
        case PLA_CANCEL:
            return;
        default:
            break;
        }
        draw_everything(convtab, len, idx);
        rb->yield();
    }
}

enum plugin_status plugin_start(const void* parameter)
{
    (void) parameter;
    MENUITEM_STRINGLIST(menu, "Unit Converter", NULL,
                        "Mass",
                        "Quit");
    bool quit = false;
    while(!quit)
    {
        switch(rb->do_menu(&menu, NULL, NULL, false))
        {
        case 0:
            /* mass */
            do_unitconv(mass_table, ARRAYLEN(mass_table));
            break;
        case 1:
            quit=true;
            break;
        default:
            break;
        }
    }
}
