#include "plugin.h"
#include "lib/display_text.h"
#include "lib/helper.h"
#include "lib/playback_control.h"
#include "lib/pluginlib_exit.h"
#include "lib/pluginlib_actions.h"

#define BUTTON_ROWS 6
#define BUTTON_COLS 5

#define REC_HEIGHT (int)(LCD_HEIGHT / (BUTTON_ROWS + 1))
#define REC_WIDTH (int)(LCD_WIDTH / BUTTON_COLS)

#define Y_7_POS (LCD_HEIGHT)           /* Leave room for the border */
#define Y_6_POS (Y_7_POS - REC_HEIGHT) /* y6 = 63 */
#define Y_5_POS (Y_6_POS - REC_HEIGHT) /* y5 = 53 */
#define Y_4_POS (Y_5_POS - REC_HEIGHT) /* y4 = 43 */
#define Y_3_POS (Y_4_POS - REC_HEIGHT) /* y3 = 33 */
#define Y_2_POS (Y_3_POS - REC_HEIGHT) /* y2 = 23 */
#define Y_1_POS (Y_2_POS - REC_HEIGHT) /* y1 = 13 */
#define Y_0_POS 0                      /* y0 = 0  */

#define X_0_POS 0                     /* x0 = 0  */
#define X_1_POS (X_0_POS + REC_WIDTH) /* x1 = 22 */
#define X_2_POS (X_1_POS + REC_WIDTH) /* x2 = 44 */
#define X_3_POS (X_2_POS + REC_WIDTH) /* x3 = 66 */
#define X_4_POS (X_3_POS + REC_WIDTH) /* x4 = 88 */
#define X_5_POS (X_4_POS + REC_WIDTH) /* x5 = 110, column 111 left blank */

#if (CONFIG_KEYPAD == IPOD_1G2G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)
#define DARTS_QUIT PLA_SELECT_REPEAT
#else
#define DARTS_QUIT PLA_CANCEL
#endif
#define DARTS_SELECT PLA_SELECT
#define DARTS_RIGHT PLA_RIGHT
#define DARTS_LEFT PLA_LEFT
#define DARTS_UP PLA_UP
#define DARTS_DOWN PLA_DOWN
#define DARTS_RRIGHT PLA_RIGHT_REPEAT
#define DARTS_RLEFT PLA_LEFT_REPEAT
#define DARTS_RUP PLA_UP_REPEAT
#define DARTS_RDOWN PLA_DOWN_REPEAT

#define RESUME_FILE PLUGIN_GAMES_DATA_DIR "/dart_scorer.save"
/* leave first line blank on bitmap display, for pause icon */
#define FIRST_LINE 1

#define NUM_PLAYERS 2
#define MAX_UNDO 100

static const struct button_mapping *plugin_contexts[] = {pla_main_ctx};

/* game data structures */
enum game_mode
{
    five,
    three
};
static struct settings_struct
{
    enum game_mode mode;
    int scores[2];
    bool turn;
    int throws;
    int history[MAX_UNDO];
    int history_ptr;
} settings;

/* temporary data */
static bool loaded = false;     /* has a save been loaded? */
int btn_row, btn_col;           /* current position index for button */
int prev_btn_row, prev_btn_col; /* previous cursor position      */
unsigned char *buttonChar[6][5] = {
    {"", "Single", "Double", "Triple", ""},
    {"1", "2", "3", "4", "5"},
    {"6", "7", "8", "9", "10"},
    {"11", "12", "13", "14", "15"},
    {"16", "17", "18", "19", "20"},
    {"", "Missed", "Bull", "Undo", ""}};
int modifier;

static int do_dart_scorer_pause_menu(void);
static void drawButtons(void);

/* First, increases *dimen1 by dimen1_delta modulo dimen1_modulo.
   If dimen1 wraps, increases *dimen2 by dimen2_delta modulo dimen2_modulo.
*/
static void move_with_wrap_and_shift(
    int *dimen1, int dimen1_delta, int dimen1_modulo,
    int *dimen2, int dimen2_delta, int dimen2_modulo)
{
    bool wrapped = false;

    *dimen1 += dimen1_delta;
    if (*dimen1 < 0)
    {
        *dimen1 = dimen1_modulo - 1;
        wrapped = true;
    }
    else if (*dimen1 >= dimen1_modulo)
    {
        *dimen1 = 0;
        wrapped = true;
    }

    if (wrapped)
    {
        /* Make the dividend always positive to be sure about the result.
           Adding dimen2_modulo does not change it since we do it modulo. */
        *dimen2 = (*dimen2 + dimen2_modulo + dimen2_delta) % dimen2_modulo;
    }
}

static void drawButtons()
{
    int i, j, w, h;
    for (i = 0; i <= 5; i++)
    {
        for (j = 0; j <= 4; j++)
        {
            unsigned char button_text[16];
            char *selected_prefix = (i == 0 && modifier > 0 && j == modifier) ? "*" : "";
            rb->snprintf(button_text, sizeof(button_text), "%s%s", selected_prefix, buttonChar[i][j]);
            rb->lcd_getstringsize(button_text, &w, &h);
            if (i == btn_row && j == btn_col) /* selected item */
                rb->lcd_set_drawmode(DRMODE_SOLID);
            else
                rb->lcd_set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
            rb->lcd_fillrect(X_0_POS + j * REC_WIDTH,
                             Y_1_POS + i * REC_HEIGHT,
                             REC_WIDTH, REC_HEIGHT + 1);
            if (i == btn_row && j == btn_col) /* selected item */
                rb->lcd_set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
            else
                rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_putsxy(X_0_POS + j * REC_WIDTH + (REC_WIDTH - w) / 2,
                           Y_1_POS + i * REC_HEIGHT + (REC_HEIGHT - h) / 2 + 1,
                           button_text);
        }
    }
    rb->lcd_set_drawmode(DRMODE_SOLID);
}

static void draw(void)
{
    rb->lcd_clear_display();

    char buf[32];

    int x = 5;
    int y = 10;
    for (int i = 0; i < NUM_PLAYERS; ++i, x = x + 95)
    {
        char *turn_marker = (i == settings.turn) ? "*" : "";
        rb->snprintf(buf, sizeof(buf), "%sPlayer %d: %d", turn_marker, i + 1, settings.scores[i]);
        rb->lcd_putsxy(x, y, buf);
    }
    int throws_x = (LCD_WIDTH / 2) - 10;
    char throws_buf[3];
    for (int i = 0; i < settings.throws; ++i, throws_x += 5)
    {
        rb->strcat(throws_buf, "1");
        rb->lcd_putsxy(throws_x, y, "|");
    }

    drawButtons();

    rb->lcd_update();
}

/*****************************************************************************
* save_game() saves the current game state.
******************************************************************************/
static void save_game(void)
{
    int fd = rb->open(RESUME_FILE, O_WRONLY | O_CREAT, 0666);
    if (fd < 0)
        return;

    rb->write(fd, &settings, sizeof(struct settings_struct));

    rb->close(fd);
    rb->lcd_update();
}

/* load_game() loads the saved game and returns load success.*/
static bool load_game(void)
{
    signed int fd;
    bool loaded = false;

    /* open game file */
    fd = rb->open(RESUME_FILE, O_RDONLY);
    if (fd < 0)
        return false;

    /* read in saved game */
    if (rb->read(fd, &settings, sizeof(struct settings_struct)) == (long)sizeof(struct settings_struct))
    {
        loaded = true;
    }

    rb->close(fd);

    return loaded;
    return false;
}

/* asks the user if they wish to quit */
static bool confirm_quit(void)
{
    const struct text_message prompt = {(const char *[]){"Are you sure?", "This will clear your current game."}, 2};
    enum yesno_res response = rb->gui_syncyesno_run(&prompt, NULL, NULL);
    if (response == YESNO_NO)
        return false;
    else
        return true;
}

/* displays the help text */
static bool do_help(void)
{

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
#endif

    rb->lcd_setfont(FONT_UI);

    static char *help_text[] = {"Dart Scorer", "", "", "Keep score of your darts game."};

    struct style_text style[] = {
        {0, TEXT_CENTER | TEXT_UNDERLINE},
    };

    return display_text(ARRAYLEN(help_text), help_text, style, NULL, true);
}

static void undo(void)
{
    if (!settings.history_ptr)
    {
        rb->splash(HZ * 2, "Out of undos!");
        return;
    }

    /* jumping back to previous player? */
    int turn = settings.throws == 3 ? !settings.turn : settings.turn;
    if (turn != settings.turn)
    {
        settings.throws = 0;
        settings.turn ^= true;
    }

    if (settings.history[settings.history_ptr - 1] >= 0)
    {
        settings.scores[turn] += settings.history[--settings.history_ptr];
        ++settings.throws;
    }
    else
    {
        /*
        negative history means we bust. negative filled for all skipped throws 
        from being bust so consume back until no more
        */
        for (; settings.throws < 3 && settings.history[settings.history_ptr - 1] < 0; --settings.history_ptr, ++settings.throws)
        {
        }
    }
}

static void init_game(bool newgame)
{
    if (newgame)
    {
        /* initialize the game context */
        modifier = 1;
        btn_row = 1;
        btn_col = 0;

        int game_mode = -1;
        MENUITEM_STRINGLIST(menu, "Game Mode", NULL, "501", "301");
        while (game_mode < 0)
        {
            switch (rb->do_menu(&menu, &game_mode, NULL, false))
            {
            case 0:
            {
                settings.mode = five;
                break;
            }
            case 1:
            {
                settings.mode = three;
                break;
            }
            }

            for (int i = 0; i < NUM_PLAYERS; ++i)
            {
                settings.scores[i] = (settings.mode == five) ? 501 : 301;
            }
            settings.turn = false;
            settings.throws = 3;
            settings.history_ptr = 0;
            rb->lcd_clear_display();
        }
    }
}

/* main game loop */
static enum plugin_status do_game(bool newgame)
{
    init_game(newgame);
    draw();

    while (1)
    {
        /* wait for button press */
        int button = pluginlib_getaction(-1, plugin_contexts, ARRAYLEN(plugin_contexts));
        unsigned char *selected = buttonChar[btn_row][btn_col];
        switch (button)
        {
        case DARTS_SELECT:
            if ((!rb->strcmp(selected, "")) || (!rb->strcmp(selected, "Single")))
                modifier = 1;
            else if (!rb->strcmp(selected, "Double"))
                modifier = 2;
            else if (!rb->strcmp(selected, "Triple"))
                modifier = 3;
            else if (!rb->strcmp(selected, "Undo"))
            {
                undo();
                continue;
            }
            else
            {
                /* main logic of score keeping */
                if (modifier == 0)
                    modifier = 1;
                int hit = (!rb->strcmp(selected, "Bull")) ? 25 : rb->atoi(selected);
                if (hit == 25 && modifier == 3)
                {
                    /* no triple bullseye! */
                    rb->splash(HZ * 2, "Triple Bull... Don't be silly!");
                    continue;
                }
                hit *= modifier;
                if (hit > settings.scores[settings.turn])
                {
                    rb->splash(HZ * 2, "Bust! End of turn.");
                    for (int i = 0; i < settings.throws; ++i)
                        settings.history[settings.history_ptr++] = -1;
                    settings.throws = 0;
                }
                else if (hit == settings.scores[settings.turn] - 1)
                {
                    rb->splash(HZ * 2, "1 left! Must checkout with a double. End of turn.");
                    for (int i = 0; i < settings.throws; ++i)
                        settings.history[settings.history_ptr++] = -1;
                    settings.throws = 0;
                }
                else if (hit == settings.scores[settings.turn] && modifier != 2)
                {
                    rb->splash(HZ * 2, "Must checkout with a double! End of turn.");
                    for (int i = 0; i < settings.throws; ++i)
                        settings.history[settings.history_ptr++] = -1;
                    settings.throws = 0;
                }
                else
                {
                    settings.scores[settings.turn] -= hit;
                    --settings.throws;
                    settings.history[settings.history_ptr++] = hit;
                    modifier = 1;
                    if (!settings.scores[settings.turn])
                        goto GAMEOVER;
                }

                if (!settings.throws)
                {
                    settings.throws = 3;
                    settings.turn ^= true;
                }
            }
            break;
        case DARTS_LEFT:
        case DARTS_RLEFT:
            move_with_wrap_and_shift(
                &btn_col, -1, BUTTON_COLS,
                &btn_row, 0, BUTTON_ROWS);
            break;
        case DARTS_RIGHT:
        case DARTS_RRIGHT:
            move_with_wrap_and_shift(
                &btn_col, 1, BUTTON_COLS,
                &btn_row, 0, BUTTON_ROWS);
            break;
#ifdef DARTS_UP
        case DARTS_UP:
        case DARTS_RUP:
#ifdef HAVE_SCROLLWHEEL
        case PLA_SCROLL_BACK:
        case PLA_SCROLL_BACK_REPEAT:
#endif
            move_with_wrap_and_shift(
                &btn_row, -1, BUTTON_ROWS,
                &btn_col, 0, BUTTON_COLS);
            break;
#endif
#ifdef DARTS_DOWN
        case DARTS_DOWN:
        case DARTS_RDOWN:
#ifdef HAVE_SCROLLWHEEL
        case PLA_SCROLL_FWD:
        case PLA_SCROLL_FWD_REPEAT:
#endif
            move_with_wrap_and_shift(
                &btn_row, 1, BUTTON_ROWS,
                &btn_col, 0, BUTTON_COLS);
            break;
#endif
        case DARTS_QUIT:
            switch (do_dart_scorer_pause_menu())
            {
            case 0: /* resume */
                break;
            case 1:
                init_game(true);
                continue;
            case 2: /* quit w/o saving */
                rb->remove(RESUME_FILE);
                return PLUGIN_ERROR;
            case 3: /* save & quit */
                save_game();
                return PLUGIN_ERROR;
                break;
            }
            break;
        default:
        {
            exit_on_usb(button); /* handle poweroff and USB */
            break;
        }
        }
        draw();
    }

GAMEOVER:
    rb->splashf(HZ * 3, "Gameover. Player %d wins!", settings.turn + 1);

    return PLUGIN_OK;
}

/* decide if this_item should be shown in the main menu */
/* used to hide resume option when there is no save */
static int mainmenu_cb(int action,
                       const struct menu_item_ex *this_item,
                       struct gui_synclist *this_list)
{
    (void)this_list;
    int idx = ((intptr_t)this_item);
    if (action == ACTION_REQUEST_MENUITEM && !loaded && (idx == 0 || idx == 5))
        return ACTION_EXIT_MENUITEM;
    return action;
}

/* show the pause menu */
static int do_dart_scorer_pause_menu(void)
{
    int sel = 0;
    MENUITEM_STRINGLIST(menu, "Dart Scorer", NULL,
                        "Resume Game",
                        "Start New Game",
                        "Playback Control",
                        "Help",
                        "Quit without Saving",
                        "Quit");
    while (1)
    {
        switch (rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
        {
            rb->splash(HZ * 2, "Resume");
            return 0;
        }
        case 1:
        {
            if (!confirm_quit())
                break;
            else
            {
                rb->splash(HZ * 2, "New Game");
                return 1;
            }
        }
        case 2:
            playback_control(NULL);
            break;
        case 3:
            do_help();
            break;
        case 4: /* quit w/o saving */
        {
            if (!confirm_quit())
                break;
            else
            {
                return 2;
            }
        }
        case 5:
            return 3;
        default:
            break;
        }
    }
}

/* show the main menu */
static enum plugin_status do_dart_scorer_menu(void)
{
    int sel = 0;
    loaded = load_game();
    MENUITEM_STRINGLIST(menu,
                        "Dart Scorer Menu",
                        mainmenu_cb,
                        "Resume Game",
                        "Start New Game",
                        "Playback Control",
                        "Help",
                        "Quit without Saving",
                        "Quit");
    while (true)
    {
        switch (rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0: /* Start new game or resume a game */
        case 1:
        {
            if (sel == 1 && loaded)
            {
                if (!confirm_quit())
                    break;
            }
            enum plugin_status ret = do_game(sel == 1);
            switch (ret)
            {
            case PLUGIN_OK:
            {
                loaded = false;
                rb->remove(RESUME_FILE);
                break;
            }
            case PLUGIN_USB_CONNECTED:
                save_game();
                return ret;
            case PLUGIN_ERROR: /* exit without menu */
                return PLUGIN_OK;
            default:
                break;
            }
            break;
        }
        case 2:
            playback_control(NULL);
            break;
        case 3:
            do_help();
            break;
        case 4:
            if (confirm_quit())
                return PLUGIN_OK;
            break;
        case 5:
            if (loaded)
                save_game();
            return PLUGIN_OK;
        default:
            break;
        }
    }
}

/* prepares for exit */
static void cleanup(void)
{
    backlight_use_settings();
}

enum plugin_status plugin_start(const void *parameter)
{
    (void)parameter;
    /* now start the game menu */
    enum plugin_status ret = do_dart_scorer_menu();
    cleanup();
    return ret;
}
