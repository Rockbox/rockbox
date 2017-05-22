#include "help.h"
#include "lib/simple_viewer.h"

void full_help(const char *name)
{
    int ch_num = -1;

    /* dirty hack */
    if(!strcmp(name, "Train Tracks"))
        name = "Tracks";

    /* search the help text for a chapter with this name */
    for(int ch = 0; ch < help_numchapters; ++ch)
    {
        char *str = help_text + help_chapteroffsets[ch];
        char *ptr = strchr(str, ':') + 1;
        const char *namep = name;
        if(*ptr++ != ' ')
            continue;

        while(*ptr == *namep && *ptr && *namep)
        {
            ptr++;
            namep++;
        }
        if(*namep == '\0' && (*ptr == '\n' || *ptr == ' ')) /* full match */
        {
            ch_num = ch;
            break;
        }
    }
    if(ch_num < 0)
    {
        rb->splashf(HZ * 2, "No topic found for `%s' (REPORT ME!)", name);
        return;
    }
    char *buf = smalloc(help_maxlen + 1);
    rb->memset(buf, 0, help_maxlen + 1);
    if(ch_num < help_numchapters - 1)
    {
        /* safe to look ahead */
        memcpy(buf, help_text + help_chapteroffsets[ch_num], help_chapteroffsets[ch_num + 1] - help_chapteroffsets[ch_num]);
    }
    else
        rb->strlcpy(buf, help_text + help_chapteroffsets[ch_num], help_maxlen + 1);

    rb->lcd_set_foreground(LCD_WHITE);
    unsigned old_bg = rb->lcd_get_background();
    rb->lcd_set_background(LCD_BLACK);
    view_text(name, buf);
    rb->lcd_set_background(old_bg);
    sfree(buf);
}
