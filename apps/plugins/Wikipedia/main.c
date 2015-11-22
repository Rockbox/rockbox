#include "plugin.h"

int font_id;

void print_text_formatted(char *text)
{
    char cur;
    int line_rem = LCD_WIDTH;
    int line_used = 0;
    int line = 0;
    for(int i = 0, m = rb->strlen(text); i != m; i++)
    {
        cur = text[i];
        if(line_rem < rb->font_get_width(rb->font_get(rb->global_status->font_id[SCREEN_MAIN]), (int)cur))
        {
            line_used = 0;
            line_rem = LCD_WIDTH;
            line = line + 20;
        }
        rb->lcd_putsxy(line_used, line, &cur);
        line_used += rb->font_get_width(rb->font_get(rb->global_status->font_id[SCREEN_MAIN]), (int)cur);
        line_rem = line_rem - rb->font_get_width(rb->font_get(rb->global_status->font_id[SCREEN_MAIN]), (int)cur);


    }
    return;
}

enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;
    font_id = rb->global_status->font_id[SCREEN_MAIN];
    int content_file = rb->open("/wp.wkc", O_RDONLY);
    int index_file = rb->open("/wp.wki", O_RDONLY);
    char idx_entry[80];
    rb->read_line(index_file, idx_entry, 80);

    int offset_to_page, page_len;
    char title[30];
    sscanf(idx_entry, "%s %d %d", title, &offset_to_page, &page_len);

    rb->lcd_set_background(LCD_WHITE);
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_clear_display();
    char test[] = "ab cdefghijklmnopqrstuvwxyz1234567890`~!@#$^&*()-_=+[]{}|;':,./<>?";
    print_text_formatted(test);
    rb->lcd_update();
    sleep(500);

    rb->close(content_file);
    rb->close(index_file);
    return PLUGIN_OK;
}
