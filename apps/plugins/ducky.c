#include "plugin.h"

/*******************************************************************************
 * The script language implemented here is an extension of DuckyScript.
 * DuckyScript as it is now is limited to simple tasks, as it lacks good flow
 * control or variable support.
 *
 * These following extensions to DuckyScript are implemented:
 *
 * Variables: there are 26 available variables, A-Z, *case-insensitive*
 *
 * "JUMP X" - jumps to line X in the file
 * "SET X Y" - loads constant Y into variable X
 * "ADD X Y" - Y = Y + X
 */

#define DEFAULT_DELAY 10
#define TOKEN_IS(str) (rb->strcmp(tok, str) == 0)
#define SEND_KEY(id) rb->usb_hid_send(HID_USAGE_PAGE_KEYBOARD_KEYPAD, id)

enum plugin_status plugin_start(const void *param)
{
    if(!param)
        rb->splash(2*HZ, "Execute a USBScript file!");
    const char *path = (const char*) param;
    int fd = rb->open(path, O_RDONLY);

    int line = 0;

    int default_delay = DEFAULT_DELAY;

    while(1)
    {
        char instr_buf[256];
        if(rb->read_line(fd, instr_buf, sizeof(instr_buf)) == 0)
            return PLUGIN_OK;
        ++line;

        char *tok = NULL, *save = NULL;

        /* execute all the commands on this line/instruction */
        do {
            tok = rb->strtok_r(instr_buf, " ", &save);

            if(TOKEN_IS("GUI")) {
                SEND_KEY(HID_KEYBOARD_LEFT_GUI);
            }
            else if(TOKEN_IS("RGUI")) {
                SEND_KEY(HID_KEYBOARD_RIGHT_GUI);
            }
            else if(TOKEN_IS("DELAY")) {
                /* delay N 100ths of a sec */
                tok = rb->strtok_r(instr_buf, " ", &save);
                rb->sleep((HZ / 100) * rb->atoi(tok));
            }
            else if(TOKEN_IS("CTRL") || TOKEN_IS("CONTROL")) {
                SEND_KEY(HID_KEYBOARD_LEFT_CONTROL);
            }
            else if(TOKEN_IS("ESC") || TOKEN_IS("ESCAPE")) {
                SEND_KEY(HID_KEYBOARD_ESCAPE);
            }
            else if(TOKEN_IS("RCTRL") || TOKEN_IS("RCONTROL")) {
                SEND_KEY(HID_KEYBOARD_RIGHT_CONTROL);
            }
            else if(TOKEN_IS("ALT") || TOKEN_IS("META")) {
                SEND_KEY(HID_KEYBOARD_LEFT_ALT);
            }
            else if(TOKEN_IS("RALT") || TOKEN_IS("RMETA")) {
                SEND_KEY(HID_KEYBOARD_RIGHT_ALT);
            }
            else if(TOKEN_IS("DEFAULT_DELAY")) {
                /* sets time between instructions, 100ths of a second */
                tok = rb->strtok_r(instr_buf, " ", &save);
                default_delay = rb->atoi(tok) * (HZ / 100);
            }
            else if(TOKEN_IS("REM") || TOKEN_IS("//")) {
                break;
            }
            else {
                rb->splashf(HZ*3, "Unknown token `%s` on line %d\n", tok, line);
                return PLUGIN_ERROR;
            }

            rb->sleep(default_delay);
        } while(tok);
    }
}
