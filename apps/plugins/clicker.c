#include "plugin.h"
#include "lib/pluginlib_actions.h"
#include "lib/pluginlib_exit.h"

#define MAX_DIGITS 128

/* data is stored like this:
   units: data[0] &0xF;
   10s:   data[0] >> 4;
   100s:  data[1] &0xF;
   100s:  data[1] >> 4;
   etc.
*/
/* 0xF in a nibble indicates no digit */
unsigned char count_data[MAX_DIGITS/2];

/* this plugin is really the epitome of overkill :D */
unsigned char read_digit(int idx)
{
    if(idx%2)
        return count_data[idx/2] >> 4;
    else
        return count_data[idx/2] &0xF;
}

void write_digit(int idx, unsigned char val)
{
    unsigned char raw=count_data[idx/2];
    if(idx%2)
    {
        /* isolate the LOW half because we're changing the HIGH half */
        raw &= 0x0F;
        raw |= val << 4;
    }
    else
    {
        raw &= 0xF0;
        raw |= val;
    }
    count_data[idx/2]=raw;
}

void increment(void)
{
    int units_digit=read_digit(0)+1;
    if(units_digit<10)
    {
        write_digit(0, units_digit);
        return;
    }
    /* there's been a carry in the units place */
    int carry=1;
    write_digit(0, 0);
    for(int i=1;i<MAX_DIGITS && carry;++i)
    {
        int read_value=read_digit(i);
        if(read_value==0xF)
            read_value=0;
        int digit=read_value + carry;
        if(digit<10)
        {
            carry=0;
        }
        else
        {
            carry=1;
            digit=0;
        }
        write_digit(i, digit);
    }
}

void draw(void)
{
    rb->lcd_clear_display();
    int next_x=0;
    for(int i=MAX_DIGITS-1;i>=0;--i)
    {
        int digit=read_digit(i);
        if(digit!=0xF)
        {
            rb->lcd_putsf(next_x, 0, "%d", digit);
            next_x++;
        }
    }
}

enum plugin_status plugin_start(const void* param){
    (void)param;
    rb->memset(count_data, 0xFF, sizeof(count_data));
    write_digit(0, 0);
    draw();
    rb->lcd_update();
    const struct button_mapping *ctxs[]={pla_main_ctx};
    int run=1;
    int f=0;

    while (run == 1) {
        int btn;
        switch (btn=pluginlib_getaction(-1,ctxs,1))
        {
        case PLA_SELECT:
            if (f != 1) {
                increment();
                draw();
                rb->lcd_update();
                f=1;
            }
            break;
        case PLA_EXIT:
        case PLA_CANCEL:
            run=0;
            break;
        default :
            f=0;
            exit_on_usb(btn);
            break;
        }
    }
    return PLUGIN_OK;
}
