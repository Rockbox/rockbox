/* Copyright (C) 2012-2015 Papavasileiou Dimitris
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include "prompt.h"

#ifdef HAVE_IOCTL
#include <sys/ioctl.h>
#endif

#include <glob.h>

#include <lualib.h>
#include <lauxlib.h>


#if LUA_VERSION_NUM == 501
#define lua_pushglobaltable(L) lua_pushvalue (L, LUA_GLOBALSINDEX)
#define LUA_OK 0
#define lua_rawlen lua_objlen
#endif

#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#else

/* This is a simple readline-like function in case readline is not
 * available. */

#define MAXINPUT 1024

static char *readline(char *prompt)
{
    char *line = NULL;
    int k;

    line = malloc (MAXINPUT);

    fputs(prompt, stdout);
    fflush(stdout);

    if (!fgets(line, MAXINPUT, stdin)) {
        return NULL;
    }

    k = strlen (line);

    if (line[k - 1] == '\n') {
        line[k - 1] = '\0';
    }

    return line;
}

#endif /* HAVE_LIBREADLINE */

#ifdef HAVE_READLINE_HISTORY
#include <readline/history.h>
#endif /* HAVE_READLINE_HISTORY */

#if LUA_VERSION_NUM == 501
#define EOF_MARKER "'<eof>'"
#else
#define EOF_MARKER "<eof>"
#endif

#define print_output(...) fprintf (stdout, __VA_ARGS__), fflush(stdout)
#define print_error(...) fprintf (stderr, __VA_ARGS__), fflush(stderr)
#define absolute(L, i) (i < 0 ? lua_gettop (L) + i + 1 : i)

#define COLOR(i) (colorize ? colors[i] : "")

static lua_State *M;
static int initialized = 0;
static char *logfile, *chunkname, *prompts[2][2], *buffer = NULL;

#ifdef SAVE_RESULTS
static int results = LUA_REFNIL, results_n = 0;
#endif

static int colorize = 1;
static const char *colors[] = {"\033[0m",
                               "\033[0;31m",
                               "\033[1;31m",
                               "\033[0;32m",
                               "\033[1;32m",
                               "\033[0;33m",
                               "\033[1;33m",
                               "\033[1m",
                               "\033[22m"};

#ifdef HAVE_LIBREADLINE

static void display_matches (char **matches, int num_matches, int max_length)
{
    print_output ("%s", COLOR(7));
    rl_display_match_list (matches, num_matches, max_length);
    print_output ("%s", COLOR(0));
    rl_on_new_line ();
}

#ifdef COMPLETE_KEYWORDS
static char *keyword_completions (const char *text, int state)
{
    static const char **c, *keywords[] = {
#if LUA_VERSION_NUM == 502
        "goto",
#endif
        "and", "break", "do", "else", "elseif", "end", "false", "for",
        "function", "if", "in", "local", "nil", "not", "or",
        "repeat", "return", "then", "true", "until", "while", NULL
    };

    int s, t;

    if (state == 0) {
        c = keywords - 1;
    }

    /* Loop through the list of keywords and return the ones that
     * match. */

    for (c += 1 ; *c ; c += 1) {
        s = strlen (*c);
        t = strlen(text);

        if (s >= t && !strncmp (*c, text, t)) {
            return strdup (*c);
        }
    }

    return NULL;
}
#endif

#ifdef COMPLETE_TABLE_KEYS

static int look_up_metatable;

static char *table_key_completions (const char *text, int state)
{
    static const char *c, *token;
    static char oper;
    static int h;

    if (state == 0) {
        h = lua_gettop(M);

        /* Scan to the beginning of the to-be-completed token. */

        for (c = text + strlen (text) - 1;
             c >= text && *c != '.' && *c != ':' && *c != '[';
             c -= 1);

        if (c > text) {
            oper = *c;
            token = c + 1;

            /* Get the iterable value, the keys of which we wish to
             * complete. */

            lua_pushliteral (M, "return ");
            lua_pushlstring (M, text, token - text - 1);
            lua_concat (M, 2);

            if (luaL_loadstring (M, lua_tostring (M, -1)) ||
                lua_pcall (M, 0, 1, 0) ||
                (lua_type (M, -1) != LUA_TUSERDATA &&
                 lua_type (M, -1) != LUA_TTABLE)) {

                lua_settop(M, h);
                return NULL;
            }
        } else {
            oper = 0;
            token = text;

            lua_pushglobaltable(M);
        }

        if (look_up_metatable) {
            /* Replace the to-be-iterated value with it's metatable
             * and set up a call to next. */

            if (!luaL_getmetafield(M, -1, "__index") ||
                (lua_type (M, -1) != LUA_TUSERDATA &&
                 lua_type (M, -1) != LUA_TTABLE)) {
                lua_settop(M, h);
                return NULL;
            }

            lua_getglobal(M, "next");
            lua_replace(M, -3);
            lua_pushnil(M);
        } else {
            /* Call the standard pairs function. */

            lua_getglobal (M, "pairs");
            lua_insert (M, -2);

            if(lua_type (M, -2) != LUA_TFUNCTION ||
               lua_pcall (M, 1, 3, 0)) {

                lua_settop(M, h);
                return NULL;
            }
        }
    }

    /* Iterate the table/userdata and generate matches. */

    while (lua_pushvalue(M, -3), lua_insert (M, -3),
           lua_pushvalue(M, -2), lua_insert (M, -4),
           lua_pcall (M, 2, 2, 0) == 0) {
        char *candidate;
        size_t l, m;
        int suppress, type, keytype;

        if (lua_isnil(M, -2)) {
            lua_settop(M, h);
            return NULL;
        }

        /* Make some notes about the value we're completing.  We'll
         * make use of them later on. */

        type = lua_type (M, -1);
        suppress = (type == LUA_TTABLE || type == LUA_TUSERDATA ||
                   type == LUA_TFUNCTION);

        keytype = LUA_TNIL;
        if (type == LUA_TTABLE) {
            lua_pushnil(M);
            if (lua_next(M, -2)) {
                keytype = lua_type (M, -2);
                lua_pop (M, 2);
            } else {
                /* There are no keys in the table so we won't want to
                 * index it.  Add a space. */

                suppress = 0;
            }
        }

        /* Pop the value, keep the key. */

        lua_pop (M, 1);

        /* We're mainly interested in strings at this point but if
         * we're completing for the table[key] syntax we consider
         * numeric keys too. */

        if (lua_type (M, -1) == LUA_TSTRING ||
            (oper == '[' && lua_type (M, -1) == LUA_TNUMBER)) {
            if (oper == '[') {
                if (lua_type (M, -1) == LUA_TNUMBER) {
                    lua_Number n;
                    int i;

                    n = lua_tonumber (M, -1);
                    i = lua_tointeger (M, -1);

                    /* If this isn't an integer key, we may as well
                     * forget about it. */

                    if ((lua_Number)i == n) {
                        l = asprintf (&candidate, "%d]", i);
                    } else {
                        continue;
                    }
                } else {
                    char q;

                    q = token[0];
                    if (q != '"' && q != '\'') {
                        q = '"';
                    }

                    l = asprintf (&candidate, "%c%s%c]",
                                  q, lua_tostring (M, -1), q);
                }
            } else {
                candidate = strdup((char *)lua_tolstring (M, -1, &l));
            }

            m = strlen(token);

            if (l >= m && !strncmp (token, candidate, m) &&
                (oper != ':' || type == LUA_TFUNCTION)
#ifdef HIDDEN_KEY_PREFIX
                && strncmp(candidate, HIDDEN_KEY_PREFIX,
                           sizeof(HIDDEN_KEY_PREFIX) - 1)
#endif
                ) {
                char *match;

                /* If the candidate has been fully typed (or
                 * previously completed) consider adding certain
                 * helpful suffixes. */
#ifndef ALWAYS_APPEND_SUFFIXES
                if (l == m) {
#endif
                    if (type == LUA_TFUNCTION) {
                        rl_completion_append_character = '('; suppress = 0;
                    } else if (type == LUA_TTABLE) {
                        if (keytype == LUA_TSTRING) {
                            rl_completion_append_character  = '.'; suppress = 0;
                        } else if (keytype != LUA_TNIL) {
                            rl_completion_append_character  = '['; suppress = 0;
                        }
                    }
#ifndef ALWAYS_APPEND_SUFFIXES
                };
#endif

                if (token > text) {
                    /* Were not completing a global variable.  Put the
                     * completed string together out of the table and
                     * the key. */

                    match = (char *)malloc ((token - text) + l + 1);
                    strncpy (match, text, token - text);
                    strcpy (match + (token - text), candidate);

                    free(candidate);
                } else {
                    /* Return the whole candidate as is, to be freed
                     * by Readline. */

                    match = candidate;
                }

                /* Suppress the newline when completing a table
                 * or other potentially complex value. */

                if (suppress) {
                    rl_completion_suppress_append = 1;
                }

                return match;
            } else {
                free(candidate);
            }
        }
    }

    lua_settop(M, h);
    return NULL;
}
#endif

#ifdef COMPLETE_MODULES
static char *module_completions (const char *text, int state)
{
    char *match = NULL;
    static int h;

    if (state == 0) {
        glob_t vector;
        const char *b, *d, *q, *s, *t, *strings[3];
        int i, n = 0, ondot, hasdot, quoted;

        hasdot = strchr(text, '.') != NULL;
        ondot = text[0] != '\0' && text[strlen(text) - 1] == '.';
        quoted = text[0] == '\'' || text[0] == '"';

#ifdef NO_MODULE_LOAD
        if(!quoted) {
            return NULL;
        }
#endif

        lua_newtable(M);
        h = lua_gettop(M);

        /* Try to load the input as a module. */

        lua_getglobal(M, "require");

        if (!lua_isfunction (M, -1)) {
            lua_settop(M, h - 1);
            return NULL;
        }

        lua_pushliteral(M, "package");

        if(lua_pcall(M, 1, 1, 0) != LUA_OK) {
            lua_settop(M, h - 1);
            return NULL;
        }

        if (!ondot && !quoted && text[0] != '\0') {
            lua_getfield(M, -1, "loaded");
            lua_pushstring(M, text);
            lua_gettable(M, -2);

            /* If it's not an already loaded module, check whether the
             * input is an available module by searching for it and/or
             * trying to load it. */

            if (!lua_toboolean(M, -1)) {
                int load = 1;

                lua_pop(M, 2);

#ifdef CONFIRM_MODULE_LOAD
                /* Look for the module as require would and ask the
                 * user whether it should be loaded or not. */

#if LUA_VERSION_NUM == 501
                lua_getfield(M, -1, "loaders");
#else
                lua_getfield(M, -1, "searchers");
#endif
                lua_pushnil(M);

                while((load = lua_next(M, -2))) {
                    lua_pushstring(M, text);
                    lua_call(M, 1, 1);

                    if (lua_isfunction(M, -1)) {
                        char c;

                        print_output ("\nLoad module '%s' (y or n)", text);

                        while ((c = tolower(rl_read_key())) != 'y' && c != 'n');

                        if (c == 'y') {
                            lua_pop(M, 3);
                            break;
                        } else {
                            print_output ("\n");
                            rl_on_new_line ();

                            /* If it was found but not loaded, return
                             * the module name as a match to avoid
                             * asking the user againg if the tab key
                             * is pressed repeatedly. */

                            lua_settop(M, h);
                            return strdup(text);
                        }
                    }

                    lua_pop(M, 1);
                }
#endif

                /* Load the model if needed. */

                if (load) {
                    lua_pushfstring (M, "%s=require(\"%s\")", text, text);

                    if (luaL_loadstring (M, lua_tostring (M, -1)) == LUA_OK &&
                        lua_pcall (M, 0, 0, 0) == LUA_OK) {
#ifdef CONFIRM_MODULE_LOAD
                        print_output (" ...loaded\n");
#else
                        print_output ("\nLoaded module '%s'.\n", text);
#endif

                        rl_on_new_line ();

                        lua_settop(M, h - 1);
                        return NULL;
                    }
                }
            } else {
                lua_settop(M, h - 1);
                return NULL;
            }

            /* Clean up but leave the package.table on the stack. */

            lua_settop(M, h + 1);
        }

        /* Look for matches in package.preload. */

        lua_getfield(M, -1, "preload");

        lua_pushnil(M);
        while(lua_next(M, -2)) {
            lua_pop(M, 1);

            if (lua_type(M, -1) == LUA_TSTRING &&
                    !strncmp(text + quoted, lua_tostring(M, -1),
                             strlen(text + quoted))) {

                lua_pushstring(M, text);
                lua_rawseti (M, h, (n += 1));
            }
        }

        lua_pop(M, 1);

        /* Get the configuration (directory, path separators, module
         * name wildcard). */

        lua_getfield(M, -1, "config");
        for (s = (char *)lua_tostring(M, -1), i = 0;
             i < 3;
             s = t + 1, i += 1) {

            t = strchr(s, '\n');
            lua_pushlstring(M, s, t - s);
            strings[i] = lua_tostring(M, -1);
        }

        lua_remove(M, -4);

        /* Get the path and cpath */

        lua_getfield(M, -4, "path");
        lua_pushstring(M, strings[1]);
        lua_getfield(M, -6, "cpath");
        lua_pushstring(M, strings[1]);
        lua_concat(M, 4);

        /* Synthesize the pattern. */

        if (hasdot) {
            luaL_gsub(M, text + quoted, ".", strings[0]);
        } else {
            lua_pushstring(M, text + quoted);
        }

        lua_pushliteral(M, "*");
        lua_concat(M, 2);

        for (b = d = lua_tostring(M, -2) ; d ; b = d + 1)  {
            size_t i;

            d = strstr(b, strings[1]);
            q = strstr(b, strings[2]);

            if (!q || q > d) {
                continue;
            }

            lua_pushlstring(M, b, d - b);
            luaL_gsub(M, lua_tostring(M, -1), strings[2],
                      lua_tostring(M, -2));

            glob(lua_tostring(M, -1), 0, NULL, &vector);

            lua_pop(M, 2);

            for (i = 0 ; i < vector.gl_pathc ; i += 1) {
                char *p = vector.gl_pathv[i];

                if (quoted) {
                    lua_pushlstring(M, text, 1);
                }

                lua_pushlstring(M, p + (q - b), strlen(p) - (d - b) + 1);

                if (hasdot) {
                    luaL_gsub(M, lua_tostring(M, -1), strings[0], ".");
                    lua_replace(M, -2);
                }

                {
                    const char *s;
                    size_t l;

                    s = lua_tolstring(M, -1, &l);

                    /* Suppress submodules named init. */

                    if (l < sizeof("init") - 1 ||
                        strcmp(s + l - sizeof("init") + 1, "init")) {

                        if (quoted) {
                            lua_pushlstring(M, text, 1);

                            lua_concat(M, 3);
                        }

                        lua_rawseti(M, h, (n += 1));
                    } else {
                        lua_pop(M, 1 + quoted);
                    }
                }
            }

            globfree(&vector);
        }

        lua_pop(M, 6);
    }

    /* Return the next match from the table of matches. */

    lua_pushnil(M);
    if (lua_next(M, -2)) {
        match = strdup(lua_tostring(M, -1));

        rl_completion_suppress_append = !(match[0] == '"' || match[0] == '\'');

        /* Pop the match. */

        lua_pushnil(M);
        lua_rawseti(M, -4, lua_tointeger(M, -3));

        /* Pop key/value. */

        lua_pop(M, 2);
    } else {
        /* Pop the empty table. */

        lua_pop(M, 1);
    }

    return match;
}
#endif

static char *generator (const char *text, int state)
{
    static int which;
    char *match = NULL;

    if (state == 0) {
        which = 0;
    }

    /* Try to complete a keyword. */

    if (which == 0) {
#ifdef COMPLETE_KEYWORDS
        if ((match = keyword_completions (text, state))) {
            return match;
        }
#endif
        which += 1;
        state = 0;
    }

    /* Try to complete a module name. */

    if (which == 1) {
#ifdef COMPLETE_MODULES
        if ((match = module_completions (text, state))) {
            return match;
        }
#endif
        which += 1;
        state = 0;
    }

    /* Try to complete a table access. */

    if (which == 2) {
#ifdef COMPLETE_TABLE_KEYS
        look_up_metatable = 0;
        if ((match = table_key_completions (text, state))) {
            return match;
        }
#endif
        which += 1;
        state = 0;
    }

    /* Try to complete a metatable access. */

    if (which == 3) {
#ifdef COMPLETE_METATABLE_KEYS
        look_up_metatable = 1;
        if ((match = table_key_completions (text, state))) {
            return match;
        }
#endif
        which += 1;
        state = 0;
    }

#ifdef COMPLETE_FILE_NAMES
    /* Try to complete a file name. */

    if (which == 4) {
        if (text[0] == '\'' || text[0] == '"') {
            match = rl_filename_completion_function (text + 1, state);

            if (match) {
                struct stat s;
                int n;

                n = strlen (match);
                stat(match, &s);

                /* If a match was produced, add the quote
                 * characters. */

                match = (char *)realloc (match, n + 3);
                memmove (match + 1, match, n);
                match[0] = text[0];

                /* If the file's a directory, add a trailing backslash
                 * and suppress the space, otherwise add the closing
                 * quote. */

                if (S_ISDIR(s.st_mode)) {
                    match[n + 1] = '/';

                    rl_completion_suppress_append = 1;
                } else {
                    match[n + 1] = text[0];
                }

                match[n + 2] = '\0';
            }
        }
    }
#endif

    return match;
}
#endif

static void finish ()
{
#ifdef HAVE_READLINE_HISTORY
    /* Save the command history on exit. */

    if (logfile) {
        write_history (logfile);
    }
#endif
}

static int traceback(lua_State *L)
{
    lua_Debug ar;
    int i;

    if (lua_isnoneornil (L, 1) ||
        (!lua_isstring (L, 1) &&
         !luaL_callmeta(L, 1, "__tostring"))) {
        lua_pushliteral(L, "(no error message)");
    }

    if (lua_gettop (L) > 1) {
        lua_replace (L, 1);
        lua_settop (L, 1);
    }

    /* Print the Lua stack. */

    lua_pushstring(L, "\n\nStack trace:\n");

    for (i = 0 ; lua_getstack (L, i, &ar) ; i += 1) {
#if LUA_VERSION_NUM == 501
        lua_getinfo(M, "Snl", &ar);
#else
        lua_getinfo(M, "Snlt", &ar);

        if (ar.istailcall) {
            lua_pushfstring(L, "\t... tail calls\n");
        }
#endif

        if (!strcmp (ar.what, "C")) {
            lua_pushfstring(L, "\t#%d %s[C]:%s in function ",
                            i, COLOR(7), COLOR(8));

            if (ar.name) {
                lua_pushfstring(L, "'%s%s%s'\n",
                                COLOR(7), ar.name, COLOR(8));
            } else {
                lua_pushfstring(L, "%s?%s\n", COLOR(7), COLOR(8));
            }
        } else if (!strcmp (ar.what, "main")) {
            lua_pushfstring(L, "\t#%d %s%s:%d:%s in the main chunk\n",
                            i, COLOR(7), ar.short_src, ar.currentline,
                            COLOR(8));
        } else if (!strcmp (ar.what, "Lua")) {
            lua_pushfstring(L, "\t#%d %s%s:%d:%s in function ",
                            i, COLOR(7), ar.short_src, ar.currentline,
                            COLOR(8));

            if (ar.name) {
                lua_pushfstring(L, "'%s%s%s'\n",
                                COLOR(7), ar.name, COLOR(8));
            } else {
                lua_pushfstring(L, "%s?%s\n", COLOR(7), COLOR(8));
            }
        }
    }

    if (i == 0) {
        lua_pushstring (L, "No activation records.\n");
    }

    lua_concat (L, lua_gettop(L));

    return 1;
}

static int execute ()
{
    int i, h_0, h, status;

#ifdef SAVE_RESULTS
    /* Get the results table, and stash it behind the to-be-executed
     * chunk. */

    lua_rawgeti(M, LUA_REGISTRYINDEX, results);
    lua_insert(M, -2);
#endif

    h_0 = lua_gettop(M);
    status = luap_call (M, 0);
    h = lua_gettop (M) - h_0 + 1;

    for (i = h ; i > 0 ; i -= 1) {
        const char *result;

        result = luap_describe (M, -i);

#ifdef SAVE_RESULTS
        lua_pushvalue (M, -i);
        lua_rawseti(M, h_0 - 1, (results_n += 1));

        print_output ("%s%s[%d]%s = %s%s\n",
                      COLOR(4), RESULTS_TABLE_NAME, results_n,
                      COLOR(3), result, COLOR(0));
#else
        if (h == 1) {
            print_output ("%s%s%s\n", COLOR(3), result, COLOR(0));
        } else {
            print_output ("%s%d%s: %s%s\n", COLOR(4), h - i + 1,
                          COLOR(3), result, COLOR(0));
        }
#endif
    }

    /* Clean up.  We need to remove the results table as well if we
     * track results. */

#ifdef SAVE_RESULTS
    lua_settop (M, h_0 - 2);
#else
    lua_settop (M, h_0 - 1);
#endif

    return status;
}

/* This is the pretty-printing related stuff. */

static char *dump;
static int length, offset, indent, column, linewidth, ancestors;

#define dump_literal(s) (check_fit(sizeof(s) - 1), \
                         strcpy (dump + offset, s), \
                         offset += sizeof(s) - 1, \
                         column += width(s))

#define dump_character(c) (check_fit(1), \
                           dump[offset] = c, \
                           offset += 1, \
                           column += 1)

static int width (const char *s)
{
    const char *c;
    int n, discard = 0;

    /* Calculate the printed width of the chunk s ignoring escape
     * sequences. */

    for (c = s, n = 0 ; *c ; c += 1) {
        if (!discard && *c == '\033') {
            discard = 1;
        }

        if (!discard) {
            n+= 1;
        }

        if (discard && *c == 'm') {
            discard = 0;
        }
    }

    return n;
}

static void check_fit (int size)
{
    /* Check if a chunk fits in the buffer and expand as necessary. */

    if (offset + size + 1 > length) {
        length = offset + size + 1;
        dump = (char *)realloc (dump, length * sizeof (char));
    }
}

static int is_identifier (const char *s, int n)
{
    int i;

    /* Check whether a string can be used as a key without quotes and
     * braces. */

    for (i = 0 ; i < n ; i += 1) {
        if (!isalpha(s[i]) &&
            (i == 0 || !isalnum(s[i])) &&
            s[i] != '_') {
            return 0;
        }
    }

    return 1;
}

static void break_line ()
{
    int i;

    check_fit (indent + 1);

    /* Add a line break. */

    dump[offset] = '\n';

    /* And indent to the current level. */

    for (i = 1 ; i <= indent ; i += 1) {
        dump[offset + i] = ' ';
    }

    offset += indent + 1;
    column = indent;
}

static void dump_string (const char *s, int n)
{
    int l;

    /* Break the line if the current chunk doesn't fit but it would
     * fit if we started on a fresh line at the current indent. */

    l = width(s);

    if (column + l > linewidth && indent + l <= linewidth) {
        break_line();
    }

    check_fit (n);

    /* Copy the string to the buffer. */

    memcpy (dump + offset, s, n);
    dump[offset + n] = '\0';

    offset += n;
    column += l;
}

static void describe (lua_State *L, int index)
{
    char *s;
    size_t n;
    int type;

    index = absolute (L, index);
    type = lua_type (L, index);

    if (luaL_getmetafield (L, index, "__tostring")) {
        lua_pushvalue (L, index);
        lua_pcall (L, 1, 1, 0);
        s = (char *)lua_tolstring (L, -1, &n);
        lua_pop (L, 1);

        dump_string (s, n);
    } else if (type == LUA_TNUMBER) {
        /* Copy the value to avoid mutating it. */

        lua_pushvalue (L, index);
        s = (char *)lua_tolstring (L, -1, &n);
        lua_pop (L, 1);

        dump_string (s, n);
    } else if (type == LUA_TSTRING) {
        int i, started, score, level, uselevel = 0;

        s = (char *)lua_tolstring (L, index, &n);

        /* Scan the string to decide how to print it. */

        for (i = 0, score = n, started = 0 ; i < (int)n ; i += 1) {
            if (s[i] == '\n' || s[i] == '\t' ||
                s[i] == '\v' || s[i] == '\r') {
                /* These characters show up better in a long sting so
                 * bias towards that. */

                score += linewidth / 2;
            } else if (s[i] == '\a' || s[i] == '\b' ||
                       s[i] == '\f' || !isprint(s[i])) {
                /* These however go better with an escaped short
                 * string (unless you like the bell or weird
                 * characters). */

                score -= linewidth / 4;
            }

            /* Check what long string delimeter level to use so that
             * the string won't be closed prematurely. */

            if (!started) {
                if (s[i] == ']') {
                    started = 1;
                    level = 0;
                }
            } else {
                if (s[i] == '=') {
                    level += 1;
                } else if (s[i] == ']') {
                    if (level >= uselevel) {
                        uselevel = level + 1;
                    }
                } else {
                    started = 0;
                }
            }
        }

        if (score > linewidth) {
            /* Dump the string as a long string. */

            dump_character ('[');
            for (i = 0 ; i < uselevel ; i += 1) {
                dump_character ('=');
            }
            dump_literal ("[\n");

            dump_string (s, n);

            dump_character (']');
            for (i = 0 ; i < uselevel ; i += 1) {
                dump_character ('=');
            }
            dump_literal ("]");
        } else {
            /* Escape the string as needed and print it as a normal
             * string. */

            dump_literal ("\"");

            for (i = 0 ; i < (int)n ; i += 1) {
                if (s[i] == '"' || s[i] == '\\') {
                    dump_literal ("\\");
                    dump_character (s[i]);
                } else if (s[i] == '\a') {
                    dump_literal ("\\a");
                } else if (s[i] == '\b') {
                    dump_literal ("\\b");
                } else if (s[i] == '\f') {
                    dump_literal ("\\f");
                } else if (s[i] == '\n') {
                    dump_literal ("\\n");
                } else if (s[i] == '\r') {
                    dump_literal ("\\r");
                } else if (s[i] == '\t') {
                    dump_literal ("\\t");
                } else if (s[i] == '\v') {
                    dump_literal ("\\v");
                } else if (isprint(s[i])) {
                    dump_character (s[i]);
                } else {
                    char t[5];
                    size_t n;

                    n = sprintf (t, "\\%03u", ((unsigned char *)s)[i]);
                    dump_string (t, n);
                }
            }

            dump_literal ("\"");
        }
    } else if (type == LUA_TNIL) {
        n = asprintf (&s, "%snil%s", COLOR(7), COLOR(8));
        dump_string (s, n);
        free(s);
    } else if (type == LUA_TBOOLEAN) {
        n = asprintf (&s, "%s%s%s",
                      COLOR(7),
                      lua_toboolean (L, index) ? "true" : "false",
                      COLOR(8));
        dump_string (s, n);
        free(s);
    } else if (type == LUA_TFUNCTION) {
        n = asprintf (&s, "<%sfunction:%s %p>",
                      COLOR(7), COLOR(8), lua_topointer (L, index));
        dump_string (s, n);
        free(s);
    } else if (type == LUA_TUSERDATA) {
        n = asprintf (&s, "<%suserdata:%s %p>",
                      COLOR(7), COLOR(8), lua_topointer (L, index));

        dump_string (s, n);
        free(s);
    } else if (type == LUA_TTHREAD) {
        n = asprintf (&s, "<%sthread:%s %p>",
                      COLOR(7), COLOR(8), lua_topointer (L, index));
        dump_string (s, n);
        free(s);
    } else if (type == LUA_TTABLE) {
        int i, l, n, oldindent, multiline, nobreak;

        /* Check if table is too deeply nested. */

        if (indent > 8 * linewidth / 10) {
            char *s;
            size_t n;

            n = asprintf (&s, "{ %s...%s }", COLOR(7), COLOR(8));
            dump_string (s, n);
            free(s);

            return;
        }

        /* Check if the table introduces a cycle by checking whether
         * it is a back-edge (that is equal to an ancestor table. */

        lua_rawgeti (L, LUA_REGISTRYINDEX, ancestors);
        n = lua_rawlen(L, -1);

        for (i = 0 ; i < n ; i += 1) {
            lua_rawgeti (L, -1, n - i);
#if LUA_VERSION_NUM == 501
            if(lua_equal (L, -1, -3)) {
#else
            if(lua_compare (L, -1, -3, LUA_OPEQ)) {
#endif
                char *s;
                size_t n;

                n = asprintf (&s, "{ %s[%d]...%s }",
                              COLOR(7), -(i + 1), COLOR(8));
                dump_string (s, n);
                free(s);
                lua_pop (L, 2);

                return;
            }

            lua_pop (L, 1);
        }

        /* Add the table to the ancestor list and pop the ancestor
         * list table. */

        lua_pushvalue (L, index);
        lua_rawseti (L, -2, n + 1);
        lua_pop (L, 1);

        /* Open the table and update the indentation level to the
         * current column. */

        dump_literal ("{ ");
        oldindent = indent;
        indent = column;
        multiline = 0;
        nobreak = 0;

        l = lua_rawlen (L, index);

        /* Traverse the array part first. */

        for (i = 0 ; i < l ; i += 1) {
            lua_pushinteger (L, i + 1);
            lua_gettable (L, index);

            /* Start a fresh line when dumping tables to make sure
             * there's plenty of room. */

            if (lua_istable (L, -1)) {
                if (!nobreak) {
                    break_line();
                }

                multiline = 1;
            }

            nobreak = 0;

            /* Dump the value and separating comma. */

            describe (L, -1);
            dump_literal (", ");

            if (lua_istable (L, -1) && i != l - 1) {
                break_line();
                nobreak = 1;
            }

            lua_pop (L, 1);
        }

        /* Now for the hash part. */

        lua_pushnil (L);
        while (lua_next (L, index) != 0) {
            if (lua_type (L, -2) != LUA_TNUMBER ||
                lua_tonumber (L, -2) != lua_tointeger (L, -2) ||
                lua_tointeger (L, -2) < 1 ||
                lua_tointeger (L, -2) > l) {

                /* Keep each key-value pair on a separate line. */

                break_line ();
                multiline  = 1;

                /* Dump the key and value. */

                if (lua_type (L, -2) == LUA_TSTRING) {
                    char *s;
                    size_t n;

                    s = (char *)lua_tolstring (L, -2, &n);

                    if(is_identifier (s, n)) {
                        dump_string (COLOR(7), strlen(COLOR(7)));
                        dump_string (s, n);
                        dump_string (COLOR(8), strlen(COLOR(8)));
                    } else {
                        dump_literal ("[");
                        describe (L, -2);
                        dump_literal ("]");
                    }
                } else {
                    dump_literal ("[");
                    describe (L, -2);
                    dump_literal ("]");
                }

                dump_literal (" = ");
                describe (L, -1);
                dump_literal (",");
            }

            lua_pop (L, 1);
        }

        /* Remove the table from the ancestor list. */

        lua_rawgeti (L, LUA_REGISTRYINDEX, ancestors);
        lua_pushnil (L);
        lua_rawseti (L, -2, n + 1);
        lua_pop (L, 1);

        /* Pop the indentation level. */

        indent = oldindent;

        if (multiline) {
            break_line();
            dump_literal ("}");
        } else {
            dump_literal (" }");
        }
    }
}

char *luap_describe (lua_State *L, int index)
{
    int oldcolorize;

#ifdef HAVE_IOCTL
    struct winsize w;

    /* Initialize the state. */

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) < 0) {
        linewidth = 80;
    } else {
        linewidth = w.ws_col;
    }
#else
    linewidth = 80;
#endif

    index = absolute (L, index);
    offset = 0;
    indent = 0;
    column = 0;

    /* Suppress colorization, to avoid escape sequences in the
     * returned strings. */

    oldcolorize = colorize;
    colorize = 0;

    /* Create a table to hold the ancestors for checking for cycles
     * when printing table hierarchies. */

    lua_newtable (L);
    ancestors = luaL_ref (L, LUA_REGISTRYINDEX);

    describe (L, index);

    luaL_unref (L, LUA_REGISTRYINDEX, ancestors);
    colorize = oldcolorize;

    return dump;
}

/* These are custom commands. */

#ifdef HAVE_LIBREADLINE
static int describe_stack (int count, int key)
{
    int i, h;

    print_output ("%s", COLOR(7));

    h = lua_gettop (M);

    if (count < 0) {
        i = h + count + 1;

        if (i > 0 && i <= h) {
            print_output ("\nValue at stack index %d(%d):\n%s%s",
                          i, -h + i - 1, COLOR(3), luap_describe (M, i));
        } else {
            print_error ("Invalid stack index.\n");
        }
    } else {
        if (h > 0) {
            print_output ("\nThe stack contains %d values.\n", h);
            for (i = 1 ; i <= h ; i += 1) {
                print_output ("\n%d(%d):\t%s", i, -h + i - 1, lua_typename(M, lua_type(M, i)));
            }
        } else {
            print_output ("\nThe stack is empty.");
        }
    }

    print_output ("%s\n", COLOR(0));

    rl_on_new_line ();

    return 0;
}
#endif

int luap_call (lua_State *L, int n) {
    int h, status;

    /* We can wind up here before reaching luap_enter, so this is
     * needed. */

    M = L;

    /* Push the error handler onto the stack. */

    h = lua_gettop(L) - n;
    lua_pushcfunction (L, traceback);
    lua_insert (L, h);

    /* Try to execute the supplied chunk and keep note of any return
     * values. */

    status = lua_pcall(L, n, LUA_MULTRET, h);

    /* Print any errors. */

    if (status != LUA_OK) {
        print_error ("%s%s%s\n", COLOR(1), lua_tostring (L, -1), COLOR(0));
        lua_pop (L, 1);
    }

    /* Remove the error handler. */

    lua_remove (L, h);

    return status;
}

void luap_setprompts(lua_State *L, const char *single, const char *multi)
{
    /* Plain, uncolored prompts. */

    prompts[0][0] = (char *)realloc (prompts[0][0], strlen (single) + 1);
    prompts[0][1] = (char *)realloc (prompts[0][1], strlen (multi) + 1);
    strcpy(prompts[0][0], single);
    strcpy(prompts[0][1], multi);

    /* Colored prompts. */

    prompts[1][0] = (char *)realloc (prompts[1][0], strlen (single) + 16);
    prompts[1][1] = (char *)realloc (prompts[1][1], strlen (multi) + 16);
#ifdef HAVE_LIBREADLINE
    sprintf (prompts[1][0], "\001%s\002%s\001%s\002",
             COLOR(6), single, COLOR(0));
    sprintf (prompts[1][1], "\001%s\002%s\001%s\002",
             COLOR(6), multi, COLOR(0));
#else
    sprintf (prompts[1][0], "%s%s%s", COLOR(6), single, COLOR(0));
    sprintf (prompts[1][1], "%s%s%s", COLOR(6), multi, COLOR(0));
#endif
}

void luap_sethistory(lua_State *L, const char *file)
{
    if (file) {
        logfile = realloc (logfile, strlen(file) + 1);
        strcpy (logfile, file);
    } else if (logfile) {
        free(logfile);
        logfile = NULL;
    }
}

void luap_setcolor(lua_State *L, int enable)
{
    /* Don't allow color if we're not writing to a terminal. */

    if (!isatty (STDOUT_FILENO) || !isatty (STDERR_FILENO)) {
        colorize = 0;
    } else {
        colorize = enable;
    }
}

void luap_setname(lua_State *L, const char *name)
{
    chunkname = (char *)realloc (chunkname, strlen(name) + 2);
    chunkname[0] = '=';
    strcpy (chunkname + 1, name);
}

void luap_getprompts(lua_State *L, const char **single, const char **multi)
{
    *single = prompts[0][0];
    *multi = prompts[0][1];
}

void luap_gethistory(lua_State *L, const char **file)
{
    *file = logfile;
}

void luap_getcolor(lua_State *L, int *enabled)
{
    *enabled = colorize;
}

void luap_getname(lua_State *L, const char **name)
{
    *name = chunkname + 1;
}

void luap_enter(lua_State *L, bool *terminate)
{
    int incomplete = 0, s = 0, t = 0, l;
    char *line, *prepended;
#ifdef SAVE_RESULTS
    int cleanup = 0;
#endif

    /* Save the state since it needs to be passed to some readline
     * callbacks. */

    M = L;

    if (!initialized) {
#ifdef HAVE_LIBREADLINE
        rl_basic_word_break_characters = " \t\n`@$><=;|&{(";
        rl_completion_entry_function = generator;
        rl_completion_display_matches_hook = display_matches;

        rl_add_defun ("lua-describe-stack", describe_stack, META('s'));
#endif

#ifdef HAVE_READLINE_HISTORY
        /* Load the command history if there is one. */

        if (logfile) {
            read_history (logfile);
        }
#endif
        if (!chunkname) {
            luap_setname (L, "lua");
        }

        if (!prompts[0][0]) {
            luap_setprompts (L, ">  ", ">> ");
        }

        atexit (finish);

        initialized = 1;
    }

#ifdef SAVE_RESULTS
    if (results == LUA_REFNIL) {
        lua_newtable(L);

#ifdef WEAK_RESULTS
        lua_newtable(L);
        lua_pushliteral(L, "v");
        lua_setfield(L, -2, "__mode");
        lua_setmetatable(L, -2);
#endif

        results = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    lua_getglobal(L, RESULTS_TABLE_NAME);
    if (lua_isnil(L, -1)) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, results);
        lua_setglobal(L, RESULTS_TABLE_NAME);

        cleanup = 1;
    }
    lua_pop(L, 1);
#endif

    while (!(*terminate) &&
           (line = readline (incomplete ?
                             prompts[colorize][1] : prompts[colorize][0]))) {
        int status;

        if (*line == '\0') {
            free(line);
            continue;
        }

        /* Add/copy the line to the buffer. */

        if (incomplete) {
            s += strlen (line) + 1;

            if (s > t) {
                buffer = (char *)realloc (buffer, s + 1);
                t = s;
            }

            strcat (buffer, "\n");
            strcat (buffer, line);
        } else {
            s = strlen (line);

            if (s > t) {
                buffer = (char *)realloc (buffer, s + 1);
                t = s;
            }

            strcpy (buffer, line);
        }

        /* Try to execute the line with a return prepended first.  If
         * this works we can show returned values. */

        l = asprintf (&prepended, "return %s", buffer);

        if (luaL_loadbuffer(L, prepended, l, chunkname) == LUA_OK) {
            execute();

            incomplete = 0;
        } else {
            lua_pop (L, 1);

            /* Try to execute the line as-is. */

            status = luaL_loadbuffer(L, buffer, s, chunkname);

            incomplete = 0;

            if (status == LUA_ERRSYNTAX) {
                const char *message;
                const int k = sizeof(EOF_MARKER) / sizeof(char) - 1;
                size_t n;

                message = lua_tolstring (L, -1, &n);

                /* If the error message mentions an unexpected eof
                 * then consider this a multi-line statement and wait
                 * for more input.  If not then just print the error
                 * message.*/

                if ((int)n > k &&
                    !strncmp (message + n - k, EOF_MARKER, k)) {
                    incomplete = 1;
                } else {
                    print_error ("%s%s%s\n", COLOR(1), lua_tostring (L, -1),
                                 COLOR(0));
                }

                lua_pop (L, 1);
            } else if (status == LUA_ERRMEM) {
                print_error ("%s%s%s\n", COLOR(1), lua_tostring (L, -1),
                             COLOR(0));
                lua_pop (L, 1);
            } else {
                /* Try to execute the loaded chunk. */

                execute ();
                incomplete = 0;
            }
        }

#ifdef HAVE_READLINE_HISTORY
        /* Add the line to the history if non-empty. */

        if (!incomplete) {
            add_history (buffer);
        }
#endif

        free (prepended);
        free (line);
    }

#ifdef SAVE_RESULTS
    if (cleanup) {
        lua_pushnil(L);
        lua_setglobal(L, RESULTS_TABLE_NAME);
    }
#endif

    print_output ("\n");
}
