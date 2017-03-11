#include "plugin.h"

#include "lib/pluginlib_actions.h"

int fd = -1;

int word_num;
off_t line_offs;

#define LINE_LEN 1024

#define MIN_WPM 100
#define MAX_WPM 1000
#define DEF_WPM 250
#define INCREMENT 25

#define FOCUS_X (7 * LCD_WIDTH / 20)
#define FOCUS_Y 0

#define WORD_COLOR LCD_BLACK
#define FOCUS_COLOR LCD_RGBPACK(255,0,0)

#define BOOKMARK_FILE VIEWERS_DATA_DIR "/speedread.dat"

char *get_next_word(void)
{
    static char line_buf[LINE_LEN];
    static int line_len = -1;
    static char *end = NULL;

next_line:

    if(line_len < 0)
    {
        line_offs = rb->lseek(fd, 0, SEEK_CUR);
        line_len = rb->read_line(fd, line_buf, LINE_LEN);
        if(line_len <= 0)
        {
            rb->splashf(HZ, "line_len is %d", line_len);
            return NULL;
        }

        char *word = rb->strtok_r(line_buf, " ", &end);

        word_num = 0;

        if(!word)
            goto next_line;
        else
            return word;
    }

    char *word = rb->strtok_r(NULL, " ", &end);
    if(!word)
    {
        /* end of line */
        line_len = -1;
        goto next_line;
    }
    ++word_num;

    return word;
}

static void reset_drawing(void)
{
    static int h = -1, w;
    if(h < 0)
        rb->lcd_getstringsize("X", &w, &h);

    /* clear word */
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_fillrect(0, 0, LCD_WIDTH, h * 2);
    rb->lcd_set_foreground(LCD_BLACK);

    /* draw frame */
    rb->lcd_fillrect(0, h * 3 / 2, LCD_WIDTH, w / 2);
    rb->lcd_fillrect(FOCUS_X - w / 4, FOCUS_Y + h * 5 / 4, w / 2, h / 2);
}

static long render_word(const char *word, int wpm)
{
    /* significant inspiration taken from spread0r */

    long base = 60 * HZ / wpm;
    long timeout = base;
    int len = rb->utf8length(word);

    if(len > 6)
        timeout += base / 5 * (len - 6);
    if(rb->strchr(word, ','))
        timeout += base / 2;
    if(rb->strcasestr(word, ".!?;"))
        timeout += 3 * base / 2;

    int focus = -1;
    for(int i = len / 5; i < len / 2; ++i)
    {
        switch(tolower(word[rb->utf8seek(word, i)]))
        {
        case 'a': case 'e': case 'i': case 'o': case 'u':
            focus = i;
            break;
        default:
            break;
        }
    }

    if(focus < 0)
        focus = len / 2;

    reset_drawing();

    /* focus char first */
    char buf[5] = { 0, 0, 0, 0, 0 };
    int idx = rb->utf8seek(word, focus);
    rb->memcpy(buf, word + idx, MIN(rb->utf8seek(word, focus + 1) - idx, 4));

    int focus_w;
    rb->lcd_getstringsize(buf, &focus_w, NULL);

    rb->lcd_set_foreground(FOCUS_COLOR);
    rb->lcd_putsxy(FOCUS_X - focus_w / 2, FOCUS_Y, buf);
    rb->lcd_set_foreground(WORD_COLOR);

    /* figure out how far left to shift */
    static char half[LINE_LEN];
    rb->strlcpy(half, word, rb->utf8seek(word, focus + 1));
    int w;
    rb->lcd_getstringsize(half, &w, NULL);

    int x = FOCUS_X - focus_w / 2 - w;

    /* first half */
    rb->lcd_putsxy(x, FOCUS_Y, half);

    /* second half */
    x = FOCUS_X + focus_w / 2;
    rb->lcd_putsxy(x, FOCUS_Y, word + rb->utf8seek(word, focus + 1));

    rb->lcd_update();
    return timeout;
}

static void init_drawing(void)
{
    rb->lcd_set_background(LCD_WHITE);
    rb->lcd_set_backdrop(NULL);
    rb->lcd_clear_display();
}

enum { NOTHING = 0, SLOWER, FASTER, FFWD, BACK, PAUSE, QUIT };

static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };

static int get_useraction(void)
{
    int button = pluginlib_getaction(0, plugin_contexts, ARRAYLEN(plugin_contexts));

    switch(button)
    {
#ifdef HAVE_SCROLLWHEEL
    case PLA_SCROLL_FWD:
    case PLA_SCROLL_FWD_REPEAT:
#else
    case PLA_UP:
#endif
        return FASTER;
#ifdef HAVE_SCROLLWHEEL
    case PLA_SCROLL_BACK:
    case PLA_SCROLL_BACK_REPEAT:
#else
    case PLA_DOWN:
#endif
        return SLOWER;
    case PLA_SELECT:
        return PAUSE;
    case PLA_CANCEL:
        return QUIT;
    default:
        return 0;
    }
}

static void wait_for_key(void)
{
    rb->splash(0, "Paused.");
    int button;
    do {
        button = pluginlib_getaction(-1, plugin_contexts, ARRAYLEN(plugin_contexts));
    } while(button != PLA_SELECT);
    rb->lcd_clear_display();
}

static void save_bookmark(const char *fname)
{
    /* copy every line except the one to be changed */
    int bookmark_fd = rb->open(BOOKMARK_FILE, O_RDONLY);
    int tmp_fd = rb->open(BOOKMARK_FILE ".tmp", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(bookmark_fd >= 0)
    {
        while(1)
        {
            /* space for the filename, 2 integers, and a null */
            static char line[MAX_PATH + 1 + 10 + 1 + 10 + 1];
            int len = rb->read_line(bookmark_fd, line, sizeof(line));
            if(len <= 0)
                break;

            char *end;
            rb->strtok_r(line, " ", &end);
            rb->strtok_r(NULL, " ", &end);
            char *bookmark_name = rb->strtok_r(NULL, "", &end);

            if(rb->strcmp(fname, bookmark_name))
            {
                /* go back and clean up after strtok */
                for(int i = 0; i < len - 1; ++i)
                    if(!line[i])
                        line[i] = ' ';

                rb->write(tmp_fd, line, len);
                rb->fdprintf(tmp_fd, "\n");
            }
        }
        rb->close(bookmark_fd);
    }
    rb->fdprintf(tmp_fd, "%ld %d %s\n", line_offs, word_num, fname);
    rb->close(tmp_fd);
    rb->rename(BOOKMARK_FILE ".tmp", BOOKMARK_FILE);
}

static void load_bookmark(const char *fname)
{
    int bookmark_fd = rb->open(BOOKMARK_FILE, O_RDONLY);
    if(bookmark_fd >= 0)
    {
        while(1)
        {
            /* space for the filename, 2 integers, and a null */
            char line[MAX_PATH + 1 + 10 + 1 + 10 + 1];
            int len = rb->read_line(bookmark_fd, line, sizeof(line));
            if(len <= 0)
                break;

            char *end;
            char *tok = rb->strtok_r(line, " ", &end);
            off_t offs = rb->atoi(tok);

            tok = rb->strtok_r(NULL, " ", &end);
            int word = rb->atoi(tok);

            char *bookmark_name = rb->strtok_r(NULL, "", &end);

            if(!rb->strcmp(fname, bookmark_name))
            {
                rb->lseek(fd, offs, SEEK_SET);
                for(int i = 0; i < word; ++i)
                    get_next_word();
                rb->close(bookmark_fd);
                rb->splash(HZ, "Resuming from bookmark...");
                return;
            }
        }
        rb->close(bookmark_fd);
    }
}

enum plugin_status plugin_start(const void *param)
{
    if(!param)
    {
        return PLUGIN_ERROR;
    }

    const char *fname = param;
    fd = rb->open_utf8(fname, O_RDONLY);

    load_bookmark(fname);

    int wpm = DEF_WPM;

    init_drawing();

    long clear = -1;

    /* main loop */
    for(;;)
    {
        switch(get_useraction())
        {
        case PAUSE:
            wait_for_key();
            break;
        case FASTER:
            if(wpm + INCREMENT <= MAX_WPM)
                wpm += INCREMENT;
            rb->splashf(0, "%d wpm", wpm);
            clear = *rb->current_tick + HZ;
            break;
        case SLOWER:
            if(wpm - INCREMENT >= MIN_WPM)
                wpm -= INCREMENT;
            rb->splashf(0, "%d wpm", wpm);
            clear = *rb->current_tick + HZ;
            break;
        case FFWD:
            break;
        case BACK:
            break;
        case QUIT:
            save_bookmark(fname);
            goto done;
        case NOTHING:
        default:
            break;
        }

        char *word = get_next_word();
        if(!word)
            break;
        if(TIME_AFTER(*rb->current_tick, clear) && clear != -1)
        {
            clear = -1;
            rb->lcd_clear_display();
        }
        int interval = render_word(word, wpm);
        rb->sleep(interval);
    }

done:
    rb->close(fd);

    return PLUGIN_OK;
}
