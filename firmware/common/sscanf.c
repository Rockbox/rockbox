#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

static inline bool isspace(char c)
{
    return (c == ' ') || (c == '\t') || (c == '\n');
}

static inline bool isdigit(char c)
{
    return (c >= '0') && (c <= '9');
}

static inline bool isxdigit(char c)
{
    return ((c >= '0') && (c <= '9'))
        || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F'));
}

static int parse_dec(int (*peek)(void *userp),
                     void (*pop)(void *userp),
                     void *userp,
                     long *vp)
{
    long v = 0;
    int n = 0;
    int minus = 0;
    char ch;
    
    if ((*peek)(userp) == '-')
    {
        (*pop)(userp);
        n++;
        minus = 1;
    }

    ch = (*peek)(userp);
    if (!isdigit(ch))
        return -1;

    do
    {
        v = v * 10 + ch - '0';
        (*pop)(userp);
        n++;
        ch = (*peek)(userp);
    } while (isdigit(ch));

    *vp = minus ? -v : v;
    return n;
}

static int parse_hex(int (*peek)(void *userp),
                     void (*pop)(void *userp),
                     void *userp,
                     unsigned long *vp)
{
    unsigned long v = 0;
    int n = 0;
    char ch;
    
    ch = (*peek)(userp);
    if (!isxdigit(ch))
        return -1;

    do
    {
        if (ch >= 'a')
            ch = ch - 'a' + 10;
        else if (ch >= 'A')
            ch = ch - 'A' + 10;
        else
            ch = ch - '0';
        v = v * 16 + ch;
        (*pop)(userp);
        n++;
        ch = (*peek)(userp);
    } while (isxdigit(ch));

    *vp = v;
    return n;
}

static int skip_spaces(int (*peek)(void *userp),
                       void (*pop)(void *userp),
                       void *userp)
{
    int n = 0;
    while (isspace((*peek)(userp))) {
        n++;
        (*pop)(userp);
    }
    return n;
}

static int scan(int (*peek)(void *userp),
                void (*pop)(void *userp),
                void *userp,
                const char *fmt,
                va_list ap)
{
    char ch;
    int n = 0;
    int n_chars = 0;
    int r;
    long lval;
    unsigned long ulval;

    while ((ch = *fmt++) != '\0')
    {
        bool literal = false;
        
        if (ch == '%')
        {
            ch = *fmt++;

            switch (ch)
            {
                case 'x':
                    n_chars += skip_spaces(peek, pop, userp);
                    if ((r = parse_hex(peek, pop, userp, &ulval)) >= 0)
                    {
                        *(va_arg(ap, unsigned int *)) = ulval;
                        n++;
                        n_chars += r;
                    }
                    else
                        return n;
                    break;
                case 'd':
                    n_chars += skip_spaces(peek, pop, userp);
                    if ((r = parse_dec(peek, pop, userp, &lval)) >= 0)
                    {
                        *(va_arg(ap, int *)) = lval;
                        n++;
                        n_chars += r;
                    }
                    else
                        return n;
                    break;
                case 'n':
                    *(va_arg(ap, int *)) = n_chars;
                    n++;
                    break;
                case 'l':
                    n_chars += skip_spaces(peek, pop, userp);
                    ch = *fmt++;
                    switch (ch)
                    {
                        case 'x':
                            if ((r = parse_hex(peek, pop, userp, &ulval)) >= 0)
                            {
                                *(va_arg(ap, unsigned long *)) = ulval;
                                n++;
                                n_chars += r;
                            }
                            else
                                return n;
                            break;
                        case 'd':
                            if ((r = parse_dec(peek, pop, userp, &lval)) >= 0)
                            {
                                *(va_arg(ap, long *)) = lval;
                                n++;
                                n_chars += r;
                            }
                            else
                                return n;
                            break;
                        case '\0':
                            return n;
                        default:
                            literal = true;
                            break;
                    }
                    break;
                case '\0':
                    return n;
                default:
                    literal = true;
                    break;
            }
        } else
            literal = true;

        if (literal)
        {
            n_chars += skip_spaces(peek, pop, userp);
            if ((*peek)(userp) != ch)
                return n;
            else
            {
                (*pop)(userp);
                n_chars++;
            }
        }
    }
    return n;
}

static int sspeek(void *userp)
{
    return **((char **)userp);
}

static void sspop(void *userp)
{
    (*((char **)userp))++;
}

int sscanf(const char *s, const char *fmt, ...)
{
    int r;
    va_list ap;
    const char *p;
    
    p = s;
    va_start(ap, fmt);
    r = scan(sspeek, sspop, &p, fmt, ap);
    va_end(ap);
    return r;
}
