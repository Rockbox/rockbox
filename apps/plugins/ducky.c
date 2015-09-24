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
 * NOTE: in the lines below, a * before a variable denotes that it is a constant
 *
 * "JUMP *X" - jumps to line X in the file
 * "JUMPZ X *Y" - if X == 0, jump to line Y
 * "JUMPLZ X *Y" - if X < 0, jump to line Y
 * "JUMPL X Y *Z" - if X < Y, jump to line Z
 * "SET X *Y" - loads constant Y into variable X
 * "MOV X Y" - Y = X
 * "SWP X Y" - X <-> Y
 * "NEG X"   - X = 0 - X
 * "ADD X Y" - Y = X + Y
 * "SUB X Y" - Y = X - Y
 * "MUL X Y" - Y = X * Y
 * "DIV X Y" - Y = X / Y
 * "MOD X Y" - Y = X % Y
 ******************************************************************************/

#define DEFAULT_DELAY 1
#define TOKEN_IS(str) (rb->strcmp(tok, str) == 0)

void add_key(int *keystate, unsigned *nkeys, int newkey)
{
    *keystate = (*keystate << 8) | newkey;
    (*nkeys)++;
}

enum plugin_status plugin_start(const void *param)
{
    if(!param)
    {
        rb->splash(2*HZ, "Execute a DuckyScript file!");
        return PLUGIN_ERROR;
    }

    const char *path = (const char*) param;
    int fd = rb->open(path, O_RDONLY);

    int line = 0;

    int default_delay = DEFAULT_DELAY;

    while(1)
    {
        char instr_buf[256];
        memset(instr_buf, 0, sizeof(instr_buf));
        if(rb->read_line(fd, instr_buf, sizeof(instr_buf)) == 0)
            return PLUGIN_OK;
        ++line;

        char *tok = NULL, *save = NULL;

        char *buf = instr_buf;

        /* the RB HID driver handles up to 4 keys simultaneously,
           1 in each byte of the id */

        int key_state = 0;
        unsigned num_keys = 0;

        /* execute all the commands on this line/instruction */
        do {
            tok = rb->strtok_r(buf, " ", &save);
            buf = NULL;

            if(!tok)
                break;

            //rb->splashf(HZ * 2, "Executing token %s", tok);

            if(TOKEN_IS("STRING")) {
                /* get the rest of the string */
                char *str = rb->strtok_r(instr_buf, "", &save);
                if(!str)
                    break;
                while(*str++)
                {
                    int shift = 0;
                    char c = *str;
                    if('A' <= c && c <= 'Z')
                        shift = 1;
                    else
                    {
                        switch(c)
                        {
                        case '~':
                        case '!':
                        case '@':
                        case '#':
                        case '$':
                        case '%':
                        case '^':
                        case '&':
                        case '*':
                        case '(':
                        case ')':
                        case '_':
                        case '+':
                        case '}':
                        case '{':
                        case '|':
                        case '"':
                        case ':':
                        case '?':
                        case '>':
                        case '<':
                            shift = 1;
                        }
                    }

                }
            }
            else if(TOKEN_IS("GUI")) {
                add_key(&key_state, &num_keys, HID_KEYBOARD_LEFT_GUI);
            }
            else if(TOKEN_IS("RGUI")) {
                add_key(&key_state, &num_keys, HID_KEYBOARD_RIGHT_GUI);
            }
            else if(TOKEN_IS("DELAY")) {
                /* delay N 100ths of a sec */
                tok = rb->strtok_r(instr_buf, " ", &save);
                rb->sleep((HZ / 100) * rb->atoi(tok));
            }
            else if(TOKEN_IS("CTRL") || TOKEN_IS("CONTROL")) {
                add_key(&key_state, &num_keys, HID_KEYBOARD_LEFT_CONTROL);
            }
            else if(TOKEN_IS("ESC") || TOKEN_IS("ESCAPE")) {
                add_key(&key_state, &num_keys, HID_KEYBOARD_ESCAPE);
            }
            else if(TOKEN_IS("RCTRL") || TOKEN_IS("RCONTROL")) {
                add_key(&key_state, &num_keys, HID_KEYBOARD_RIGHT_CONTROL);
            }
            else if(TOKEN_IS("ALT") || TOKEN_IS("META")) {
                add_key(&key_state, &num_keys, HID_KEYBOARD_LEFT_ALT);
            }
            else if(TOKEN_IS("RALT") || TOKEN_IS("RMETA")) {
                add_key(&key_state, &num_keys, HID_KEYBOARD_RIGHT_ALT);
            }
            else if(TOKEN_IS("DEFAULT_DELAY")) {
                /* sets time between instructions, 100ths of a second */
                tok = rb->strtok_r(instr_buf, " ", &save);
                default_delay = rb->atoi(tok) * (HZ / 100);
            }
            else if(TOKEN_IS("D")) {
                add_key(&key_state, &num_keys, HID_KEYBOARD_D);
            }
            else if(TOKEN_IS("REM") || TOKEN_IS("//")) {
                /* break out of the do-while */
                break;
            }
            else {
                rb->splashf(HZ*3, "ERROR: Unknown token `%s` on line %d\n", tok, line);
                goto done;
            }

            /* all the key states are packed into an int, therefore the total
               number of keys down at one time can't exceed sizeof(int) */
            if(num_keys > sizeof(int))
            {
                rb->splashf(2*HZ, "ERROR: Too many keys depressed on line %d", line);
                goto done;
            }

            rb->usb_hid_send(HID_USAGE_PAGE_KEYBOARD_KEYPAD, key_state);

            rb->sleep(default_delay);
            rb->yield();
        } while(tok);
    }

done:

    rb->close(fd);
    return PLUGIN_OK;
}
