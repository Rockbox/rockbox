#include "plugin.h"
#include "lib/pluginlib_actions.h"
#include <stdbool.h>
/* data headers go below */
#include "length.h"
#include "mass.h"
#include "temperature.h"

#define MAX_PRECISION 100000
#define LOG_MAXPREC 5

static void convert_units(struct unit_t *convtab, const size_t len, size_t idx)
{
    for(size_t i=0;i<len;++i)
    {
        if(i != idx)
        {
            convtab[i].value = ((double)convtab[idx].value + (double)convtab[i].offset) / (double)convtab[i].ratio * (double)convtab[idx].ratio;
        }
    }
}

static void draw_everything(struct unit_t *convtab, const size_t len, size_t idx)
{
    convert_units(convtab, len, idx);
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_clear_display();
    for(size_t i=0;i<len;++i)
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
        int64_t fracpart = convtab[i].value % convtab[0].ratio;
        if(fracpart / (UNIT / MAX_PRECISION))
        {
            rb->lcd_putsf(0, i, "%llu.%0*llu %s",
                          convtab[i].value / convtab[0].ratio,
                          LOG_MAXPREC,
                          fracpart / (UNIT / MAX_PRECISION),
                          convtab[i].abbr);
        }
        else
        {
            rb->lcd_putsf(0, i, "%llu %s",
                          convtab[i].value / convtab[0].ratio,
                          convtab[i].abbr);
        }
    }
    rb->lcd_update();
}

static void do_unitconv(struct unit_t *convtab, const size_t len)
{
    const struct button_mapping *plugin_contexts[] = { pla_main_ctx };
    size_t idx = 0;
    bool quit = false;
    while(!quit)
    {
        draw_everything(convtab, len, idx);
        switch(pluginlib_getaction(-1, plugin_contexts, ARRAYLEN(plugin_contexts)))
        {
        case PLA_DOWN:
        case PLA_DOWN_REPEAT:
            if(idx++ >= len)
                idx=0;
            break;
        case PLA_UP:
        case PLA_UP_REPEAT:
            if((signed int)idx-- < 0)
                idx = len-1;
            break;
        case PLA_RIGHT:
        case PLA_RIGHT_REPEAT:
            convtab[idx].value += convtab[idx].step;
            break;
        case PLA_LEFT:
        case PLA_LEFT_REPEAT:
            convtab[idx].value -= convtab[idx].step;
            break;
        case PLA_CANCEL:
            return;
        default:
            break;
        }
        rb->yield();
    }
}

enum plugin_status plugin_start(const void* parameter)
{
    (void) parameter;
    MENUITEM_STRINGLIST(menu, "Unit Converter", NULL,
                        "Length",
                        "Mass",
                        "Temperature",
                        "Quit");
    bool quit = false;
    while(!quit)
    {
        switch(rb->do_menu(&menu, NULL, NULL, false))
        {
        case 0:
            /* length */
            do_unitconv(length_table, ARRAYLEN(length_table));
            break;
        case 1:
            /* mass */
            do_unitconv(mass_table, ARRAYLEN(mass_table));
            break;
        case 2:
            /* temperature */
            do_unitconv(temperature_table, ARRAYLEN(temperature_table));
            break;
        case 3:
            quit = true;
            break;
        default:
            break;
        }
    }
    return PLUGIN_OK;
}
