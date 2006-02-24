#ifndef _GUI_YESNO_H_
#define _GUI_YESNO_H_

#include "screen_access.h"
#include "textarea.h"

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define YESNO_OK       BUTTON_SELECT
#define YESNO_RC_OK    BUTTON_RC_MENU

#elif CONFIG_KEYPAD == RECORDER_PAD
#define YESNO_OK       BUTTON_PLAY
#define YESNO_RC_OK    BUTTON_RC_PLAY

#elif CONFIG_KEYPAD == PLAYER_PAD
#define YESNO_OK       BUTTON_PLAY
#define YESNO_RC_OK    BUTTON_RC_PLAY

#elif CONFIG_KEYPAD == ONDIO_PAD
#define YESNO_OK       BUTTON_RIGHT

#elif CONFIG_KEYPAD == GMINI100_PAD
#define YESNO_OK       BUTTON_PLAY

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)
#define YESNO_OK       BUTTON_PLAY

#elif (CONFIG_KEYPAD == IRIVER_IFP7XX_PAD)
#define YESNO_OK       BUTTON_RIGHT

#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)
#define YESNO_OK       BUTTON_RIGHT

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define YESNO_OK       BUTTON_RIGHT

#endif
enum yesno_res
{
    YESNO_YES,
    YESNO_NO,
    YESNO_USB
};

struct gui_yesno
{
    struct text_message * main_message;
    struct text_message * result_message[2];

    struct screen * display;
};

/*
 * Initializes the yesno asker
 *  - yn : the yesno structure
 *  - main_message : the question the user has to answer
 *  - yes_message : message displayed if answer is 'yes'
 *  - no_message : message displayed if answer is 'no'
 */
extern void gui_yesno_init(struct gui_yesno * yn,
                           struct text_message * main_message,
                           struct text_message * yes_message,
                           struct text_message * no_message);

/*
 * Attach the yesno to a screen
 *  - yn : the yesno structure
 *  - display : the screen to attach
 */
extern void gui_yesno_set_display(struct gui_yesno * yn,
                                 struct screen * display);

/*
 * Draws the yesno
 *  - yn : the yesno structure
 */
extern void gui_yesno_draw(struct gui_yesno * yn);

/*
 * Draws the yesno result
 *  - yn : the yesno structure
 *  - result : the result tha must be displayed :
 *    YESNO_NO if no
 *    YESNO_YES if yes
 */
extern bool gui_yesno_draw_result(struct gui_yesno * yn, enum yesno_res result);

/*
 * Runs the yesno asker :
 * it will display the 'main_message' question, and wait for user keypress
 * PLAY means yes, other keys means no
 *  - main_message : the question the user has to answer
 *  - yes_message : message displayed if answer is 'yes'
 *  - no_message : message displayed if answer is 'no'
 */
extern enum yesno_res gui_syncyesno_run(
                           struct text_message * main_message,
                           struct text_message * yes_message,
                           struct text_message * no_message);
#endif /* _GUI_YESNO_H_ */
