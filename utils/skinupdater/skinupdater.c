#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "tag_table.h"

#define PUTCH(out, c) fprintf(out, "%c", c)
extern struct tag_info legal_tags[];
/* dump "count" args to output replacing '|' with ',' except after the last count.
 * return the amount of chars read. (start+return will be after the last | )
 */
int dump_arg(FILE* out, const char* start, int count, bool close)
{
    int l = 0;
    while (count)
    {
        if (start[l] == '|')
        {
            if (count > 1)
            {
                printf(",");
            } else if (close) {
                printf(")");
            }
            count--;
        } else {
            PUTCH(out, start[l]);
        }
        l++;
    }
    return l;
}
#define MATCH(s) (!strcmp(tag->name, s))
int parse_tag(FILE* out, const char* start)
{
    struct tag_info *tag;
    int len = 0;
    for(tag = legal_tags; 
        tag->name[0] && strncmp(start, tag->name, strlen(tag->name)) != 0;
        tag++) ;
    if (!tag->name[0])
        return -1;
    if (tag->params[0] == '\0')
    {
        fprintf(out, "%s", tag->name);
        return strlen(tag->name);
    }
    fprintf(out, "%s", tag->name);
    len += strlen(tag->name);
    start += len;
    /* handle individual tags which accept params */
    if (MATCH("bl") || MATCH("pb") || MATCH("pv"))
    {
        start++; len++;
        if (*start == '|')
        {
            PUTCH(out, '(');
            /* TODO: need to verify that we are actually using the long form... */
            len += dump_arg(out, start, 5, true);
        }
    }
    else if (MATCH("d") || MATCH("D") || MATCH("mv") || MATCH("pS") || MATCH("pE") || MATCH("t") || MATCH("Tl"))
    {
        char temp[8] = {'\0'};
        int i = 0;
        /* tags which maybe have a number after them */
        while ((*start >= '0' && *start <= '9') || *start == '.')
        {
            temp[i++] = *start++;
        }
        if (i!= 0)
        {
            fprintf(out, "(%s)", temp);
            len += i;
        }
    }
    else if (MATCH("xl"))
    {
        PUTCH(out, '(');
        len += 1+dump_arg(out, start+1, 5, true);
    }
    else if (MATCH("xd"))
    {
        /* NOTE: almost certainly needs work */
        PUTCH(out, '(');
        PUTCH(out, *start++); len++;
        if ((*start >= 'a' && *start <= 'z') ||
            (*start >= 'A' && *start <= 'Z'))
        PUTCH(out, *start); len++;
        PUTCH(out, ')');
    }
    else if (MATCH("x"))
    {
        PUTCH(out, '(');
        len += 1+dump_arg(out, start+1, 4, true);
    }
    else if (MATCH("Fl"))
    {
        PUTCH(out, '(');
        len += 1+dump_arg(out, start+1, 2, true);
    }
    else if (MATCH("Cl"))
    {
        PUTCH(out, '(');
        len += 1+dump_arg(out, start+1, 4, true);
    }
    else if (MATCH("Vd") || MATCH("VI"))
    {
        PUTCH(out, '(');
        PUTCH(out, *start); len++;
        PUTCH(out, ')');
    }
    else if (MATCH("Vp"))
    {
        /* NOTE: almost certainly needs work */
        PUTCH(out, '(');
        len += 1+dump_arg(out, start+1, 3, true);
    }
    else if (MATCH("Vl") || MATCH("Vi"))
    {
        PUTCH(out, '(');
        len += 1+dump_arg(out, start+1, 8, true);
    }
    else if (MATCH("V"))
    {
        PUTCH(out, '(');
        len += 1+dump_arg(out, start+1, 7, true);
    }
    else if (MATCH("X"))
    {
        if (*start+1 == 'd')
        {
            fprintf(out, "(d)");
            len ++;
        }
        else
        {
            PUTCH(out, '(');
            len += 1+dump_arg(out, start+1, 1, true);
        }
    }
    else if (MATCH("St") || MATCH("Sx"))
    {
        PUTCH(out, '(');
        len += 1+dump_arg(out, start+1, 1, true);
    }
    
    else if (MATCH("T"))
    {
        PUTCH(out, '(');
        len += 1+dump_arg(out, start+1, 5, true);
    }
    return len;
}
        
void parse_text(const char* in, FILE* out)
{
    const char* end = in+strlen(in);
top:
    while (in < end && *in)
    {
        if (*in == '%')
        {
            PUTCH(out, *in++);
            switch(*in)
            {

                case '%':
                case '<':
                case '|':
                case '>':
                case ';':
                case '#':
                    PUTCH(out, *in++);
                    goto top;
                    break;
                case '?':
                    PUTCH(out, *in++);
                    break;
            }
            in += parse_tag(out, in);
        }
        else 
        {
            PUTCH(out, *in++);
        }            
    }
}

int main(int argc, char* argv[])
{
    parse_text("%s%?it<%?in<%in. |>%it|%fn>\n"
            "%s%?ia<%ia|%?d2<%d2|(root)>>\n"
            "%s%?id<%id|%?d1<%d1|(root)>> %?iy<(%iy)|>\n\n"
            "%al%pc/%pt%ar[%pp:%pe]\n"
            "%fbkBit %?fv<avg|> %?iv<(id3v%iv)|(no id3)>\n"
            "%pb\n%pm\n", stdout);
    return 0;
}
