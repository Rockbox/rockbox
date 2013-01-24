#include "plugin.h"
#include <tlsf.h>
#include "console.h"

static int console_lines = 0;
static char **console_lines_list = NULL;

static const char* list_get_name_cb(int selected_item, void* data,
                                    char* buf, size_t buf_len)
{
    (void)data;

    rb->strlcpy(buf, console_lines_list[selected_item], buf_len);
    return buf;
}

static int add_console_line(char *message)
{
    console_lines_list = tlsf_realloc(console_lines_list,
                                      (console_lines + 1)*sizeof(char *));

    if (console_lines_list == NULL)
        return -1;

    console_lines_list[console_lines++] = message;
    return 0;
}

int console_puts(const char *s)
{
    static char *lp, *line = NULL;
    const char *sp = s;

    if (line == NULL)
    {
        /* new line */
        line = (char *)tlsf_malloc(MAX_LINE_LEN);

        if (line == NULL)
            return -1;

        lp = line;
    }

    while (*sp != '\0')
    {
        if (*sp == '\n' || (lp - line) > (MAX_LINE_LEN - 1))
        {
            *lp = '\0';
            line = tlsf_realloc(line, (lp - line + 1));

            if (add_console_line(line))
                return -1;

            line = NULL;
            return 0;
        }
        *lp++ = *sp++;
    }

    return 0;
}

void show_console(void)
{
    struct gui_synclist console;
    int button;

    rb->gui_synclist_init(&console,list_get_name_cb, 0, false, 1, NULL);
    rb->gui_synclist_set_icon_callback(&console, NULL);
    rb->gui_synclist_set_nb_items(&console, console_lines);
    rb->gui_synclist_limit_scroll(&console, true);
    rb->gui_synclist_select_item(&console, console_lines-1);
    rb->gui_synclist_draw(&console);

    while(1)
    {
        button = rb->get_action(CONTEXT_LIST, TIMEOUT_BLOCK);

        if (rb->gui_synclist_do_button(&console, &button, LIST_WRAP_OFF))
            continue;

        if (button == ACTION_STD_CANCEL)
            break;
    }

}
