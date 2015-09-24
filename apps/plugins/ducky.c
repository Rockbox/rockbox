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
#define STRING_DELAY 1
#define TOKEN_IS(str) (rb->strcmp(tok, str) == 0)

void add_key(int *keystate, unsigned *nkeys, int newkey)
{
    *keystate = (*keystate << 8) | newkey;
    if(nkeys)
        (*nkeys)++;
}

struct char_mapping {
    char c;
    int key;
};

struct char_mapping shift_tab[] = {
    { '~', HID_KEYBOARD_BACKTICK  },
    { '!', HID_KEYBOARD_1 },
    { '@', HID_KEYBOARD_2 },
    { '#', HID_KEYBOARD_3 },
    { '$', HID_KEYBOARD_4 },
    { '%', HID_KEYBOARD_5 },
    { '^', HID_KEYBOARD_6 },
    { '&', HID_KEYBOARD_7 },
    { '*', HID_KEYBOARD_8 },
    { '(', HID_KEYBOARD_9 },
    { ')', HID_KEYBOARD_0 },
    { '_', HID_KEYBOARD_HYPHEN },
    { '+', HID_KEYBOARD_EQUAL_SIGN },
    { '}', HID_KEYBOARD_RIGHT_BRACKET },
    { '{', HID_KEYBOARD_LEFT_BRACKET },
    { '|', HID_KEYBOARD_BACKSLASH },
    { '"', HID_KEYBOARD_QUOTE },
    { ':', HID_KEYBOARD_SEMICOLON },
    { '?', HID_KEYBOARD_SLASH },
    { '>', HID_KEYBOARD_DOT },
    { '<', HID_KEYBOARD_COMMA },
};

struct char_mapping char_tab[] = {
    { ' ', HID_KEYBOARD_SPACEBAR },
    { '`', HID_KEYBOARD_BACKTICK },
    { '-', HID_KEYBOARD_HYPHEN },
    { '=', HID_KEYBOARD_EQUAL_SIGN },
    { '[', HID_KEYBOARD_LEFT_BRACKET },
    { ']', HID_KEYBOARD_RIGHT_BRACKET },
    { '\\', HID_KEYBOARD_BACKSLASH },
    { '\'', HID_KEYBOARD_QUOTE },
    { ';', HID_KEYBOARD_SEMICOLON },
    { '/', HID_KEYBOARD_SLASH },
    { ',', HID_KEYBOARD_COMMA },
    { '.', HID_KEYBOARD_DOT },
};

void add_char(int *keystate, unsigned *nkeys, char c)
{
    if('a' <= c && c <= 'z')
    {
        add_key(keystate, nkeys, c - 'a' + HID_KEYBOARD_A);
    }
    else if('A' <= c && c <= 'Z')
    {
        add_key(keystate, nkeys, HID_KEYBOARD_LEFT_SHIFT);
        add_key(keystate, nkeys, c - 'A' + HID_KEYBOARD_A);
    }
    else if('0' <= c && c <= '9')
    {
        if(c == '0')
            add_key(keystate, nkeys, HID_KEYBOARD_0);
        else
            add_key(keystate, nkeys, c - '1' + HID_KEYBOARD_1);
    }
    else
    {
        /* search the character table */
        for(unsigned int i = 0; i < ARRAYLEN(char_tab); ++i)
        {
            if(char_tab[i].c == c)
            {
                add_key(keystate, nkeys, char_tab[i].key);
                return;
            }
        }

        /* search the shift-mapping table */
        for(unsigned int i = 0; i < ARRAYLEN(shift_tab); ++i)
        {
            if(shift_tab[i].c == c)
            {
                add_key(keystate, nkeys, HID_KEYBOARD_LEFT_SHIFT);
                add_key(keystate, nkeys, shift_tab[i].key);
                return;
            }
        }

        rb->splashf(2 * HZ, "WARNING: invalid character '%c'", c);
    }
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

    int default_delay = DEFAULT_DELAY, string_delay = STRING_DELAY;

    while(1)
    {
        char instr_buf[256];
        memset(instr_buf, 0, sizeof(instr_buf));
        if(rb->read_line(fd, instr_buf, sizeof(instr_buf)) == 0)
            goto done;
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

            if(rb->strlen(tok) == 1) {
                add_char(&key_state, &num_keys, tok[0]);
            }
            else if(TOKEN_IS("STRING")) {
                /* get the rest of the string */
                char *str = rb->strtok_r(NULL, "", &save);
                rb->splashf(HZ, "string %s", str);
                if(!str)
                    break;
                while(*str)
                {
                    int string_state = 0;
                    if(!*str)
                        break;
                    add_char(&string_state, NULL, *str);
                    rb->usb_hid_send(HID_USAGE_PAGE_KEYBOARD_KEYPAD, string_state);
                    ++str;
                    rb->sleep(string_delay);
                }
                continue;
            }
            else if(TOKEN_IS("GUI")) {
                add_key(&key_state, &num_keys, HID_KEYBOARD_LEFT_GUI);
            }
            else if(TOKEN_IS("RGUI")) {
                add_key(&key_state, &num_keys, HID_KEYBOARD_RIGHT_GUI);
            }
            else if(TOKEN_IS("DELAY")) {
                /* delay N 100ths of a sec */
                tok = rb->strtok_r(NULL, " ", &save);
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
                tok = rb->strtok_r(NULL, " ", &save);
                default_delay = rb->atoi(tok) * (HZ / 100);
            }
            else if(TOKEN_IS("STRING_DELAY")) {
                tok = rb->strtok_r(NULL, " ", &save);
                string_delay = rb->atoi(tok) * (HZ / 100);
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
