/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: tag_table.c 26346 2010-05-28 02:30:27Z jdgordon $
 *
 * Copyright (C) 2010 Jonathan Gordon
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "tag_table.h"

#define PUTCH(out, c) fprintf(out, "%c", c)
extern struct tag_info legal_tags[];

char images_with_subimages[100];
int image_count = 0;

/** Command line setting **/
bool is_mono_display = false;
bool use_new_vp_tags = true;


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
                PUTCH(out, ',');
            } else if (close) {
                PUTCH(out, ')');
            }
            count--;
        } else {
            if (find_escape_character(start[l]))
            {
                PUTCH(out, '%');
            }
            PUTCH(out, start[l]);
        }
        l++;
    }
    return l;
}

int dump_viewport_tags(FILE* out, const char* start)
{
    int len = 0;
    if (is_mono_display)
    {
        return dump_arg(out, start, 5, true);
    }
    else
    {
        int arg_count = use_new_vp_tags?5:7;
        len += dump_arg(out, start, arg_count, true);
        if (!use_new_vp_tags)
            return len;
        if (start[len] != '-')
        {
            fprintf(out, "%%Vf(");
            len += dump_arg(out, start+len, 1, true);
        }
        else
        {
            while (start[len++] != '|');
        }
        if (start[len] != '-')
        {
            fprintf(out, "%%Vb(");
            len += dump_arg(out, start+len, 1, true);
        }
        else
        {
            while (start[len++] != '|');
        }
    }
    return len;
}

#define MATCH(s) (!strcmp(tag->name, s))
int parse_tag(FILE* out, const char* start, bool in_conditional)
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
    if (!strcmp(tag->name, "C"))
    {
        fprintf(out, "Cd");
        return 1;
    }
        
    fprintf(out, "%s", tag->name);
    len += strlen(tag->name);
    start += len;
    /* handle individual tags which accept params */
    if ((MATCH("bl") || MATCH("pb") || MATCH("pv")) && !in_conditional)
    {
        if (*start == '|')
        {
            int i=0;
            char filename[128];
            len++; start++;
            PUTCH(out, '(');
            /* |file|x|y|width|height| -> (x,y,width,height,file) */
            while (start[i] != '|')
            {
                filename[i] = start[i];
                i++;
            }
            filename[i] = '\0';
            len +=i+1;
            start += i+1;
            /* TODO: need to verify that we are actually using the long form... */
            len += dump_arg(out, start, 4, false);
            if (i>0)
            {
                fprintf(out, ",%s", filename);
            }
            PUTCH(out, ')');
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
        char label = start[1];
        PUTCH(out, '(');
        int read = 1+dump_arg(out, start+1, 4, false);
        len += read;
        start += read;
        if (*start>= '0' && *start <= '9')
        {
            images_with_subimages[image_count++] = label;
            PUTCH(out, ',');
            len += dump_arg(out, start, 1, false);
        }
        PUTCH(out, ')');
    }
    else if (MATCH("xd"))
    {
        char label = start[0];
        int i=0;
        while (i<image_count)
        {
            if (images_with_subimages[i] == label)
                break;
            i++;
        }
        PUTCH(out, '(');
        PUTCH(out, *start++); len++;
        if (i<image_count && 
            ((*start >= 'a' && *start <= 'z') ||
             (*start >= 'A' && *start <= 'Z')))
        {
            PUTCH(out, *start); len++;
        }
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
        int read;
        char xalign = '\0', yalign = '\0';
        PUTCH(out, '(');
        read = 1+dump_arg(out, start+1, 2, false);
        len += read;
        start += read;
        switch (tolower(*start))
        {
            case 'l':
            case 'c':
            case 'r':
            case '+':
            case '-':
                xalign = *start;
                len++;
                start++;
                break;
            case 'd':
            case 'D':
            case 'i':
            case 'I':
            case 's':
            case 'S':
                len++;
                start++;
                break;
        }
        PUTCH(out,',');
        read = dump_arg(out, start, 1, false);
        len += read;
        start += read;
        switch (tolower(*start))
        {
            case 't':
            case 'c':
            case 'b':
            case '+':
            case '-':
                yalign = *start;
                len++;
                start++;
                break;
            case 'd':
            case 'D':
            case 'i':
            case 'I':
            case 's':
            case 'S':
                len++;
                start++;
                break;
        }
        PUTCH(out,',');
        read = dump_arg(out, start, 1, false);
        if (xalign)
        {
            if (xalign == '-')
                xalign = 'l';
            if (xalign == '+')
                xalign = 'r';
            fprintf(out, ",%c", xalign); 
        }
        if (yalign)
        {
            if (yalign == '-')
                yalign = 't';
            if (yalign == '+')
                yalign = 'b';
            fprintf(out, ",%s%c", xalign?"":"-,", yalign); 
        }
        PUTCH(out, ')');
        len += read;
    }
    else if (MATCH("Vd") || MATCH("VI"))
    {
        PUTCH(out, '(');
        PUTCH(out, *start); len++;
        PUTCH(out, ')');
    }
    else if (MATCH("Vp"))
    {
        int read;
        /* NOTE: almost certainly needs work */
        PUTCH(out, '(');
        read = 1+dump_arg(out, start+1, 1, false);
        PUTCH(out, ',');
        while (start[read] != '|')
        {
            PUTCH(out, start[read++]);
        }
        PUTCH(out, ',');
        read++;
        while (start[read] != '|')
        {
            PUTCH(out, start[read++]);
        }
        PUTCH(out, ')');
        read++;
        len += read;
    }
    else if (MATCH("Vi"))
    {
        int read = 1;
        
        PUTCH(out, '(');
        if ((start[1] >= 'a' && start[1] <= 'z') ||
            (start[1] >= 'A' && start[1] <= 'Z'))
        {
            read = 1+dump_arg(out, start+1, 1, false);
        }
        else
        {
            PUTCH(out, '-');
        }
        PUTCH(out, ',');
        len += read + dump_viewport_tags(out, start+read);
    }
    else if (MATCH("Vl") || MATCH("Vi"))
    {
        int read;
        PUTCH(out, '(');
        read = 1+dump_arg(out, start+1, 1, false);
        PUTCH(out, ',');
        len += read + dump_viewport_tags(out, start+read);
    }
    else if (MATCH("V"))
    {
        PUTCH(out, '(');
        len += 1+dump_viewport_tags(out, start+1);
    }
    else if (MATCH("X"))
    {
        if (*start == 'd')
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
    int level = 0;
    int len;
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
                case ')':
                case '(':
                case ',':
                    PUTCH(out, *in++);
                    goto top;
                    break;
                case '?':
                    if (in[1] == 'C' && in[2] == '<')
                    {
                        fprintf(out, "?C");
                        in += 2;
                        goto top;
                    }
                    else
                    {
                        PUTCH(out, *in++);
                    }
                    break;
            }
            len = parse_tag(out, in, level>0);
            if (len < 0)
            {
                PUTCH(out, *in++);
            }
            else
            {
                in += len;
            }
        }
        else if (*in == '<')
        {
            level++;
            PUTCH(out, *in++);
        }
        else if (*in == '>')
        {
            level--;
            PUTCH(out, *in++);
        }
        else if (*in == '|')
        {
            if (level == 0)
            {
                PUTCH(out, '%');
            }
            PUTCH(out, *in++);
        }
        else if (*in == '#')
        {
            while (*in && *in != '\n')
            {
                PUTCH(out, *in++);
            }
        }
        else if (*in == ';')
        {
            PUTCH(out, *in++);
        }
        else 
        {
            if (find_escape_character(*in))
            {
                PUTCH(out, '%');
            }
            PUTCH(out, *in++);
        }            
    }
}

int main(int argc, char* argv[])
{
    char buffer[10*1024], temp[512];
    FILE *in, *out = stdout;
    int filearg = 1, i=0;
    if( (argc < 2) ||
        strcmp(argv[1],"-h") == 0 ||
        strcmp(argv[1],"--help") == 0 )
    {
        printf("Usage: %s [OPTIONS] infile [outfile]\n", argv[0]);
        printf("\nOPTIONS:\n");
        printf("\t-c\tDon't use new viewport colour tags (non-mono displays only)\n");
        printf("\t-m\tSkin is for a mono display (different viewport tags)\n");
        return 0;
    }
    
    while ((argc > filearg) && argv[filearg][0] == '-')
    {
        i=1;
        while (argv[filearg][i])
        {
            switch(argv[filearg][i])
            {
                case 'c': /* disable new colour tags */
                    use_new_vp_tags = false;
                    break;
                case 'm': /* skin is for a mono display */
                    is_mono_display = true;
                    break;
            }
            i++;
        }
        filearg++;
    }
    if (argc == filearg)
    {
        printf("Missing input filename\n");
        return 1;
    }
    
    in = fopen(argv[filearg], "r");
    if (!in)
        return 1;
    while (fgets(temp, 512, in))
        strcat(buffer, temp);
    fclose(in);
    filearg++;
    
    if (argc > filearg)
    {
        out = fopen(argv[filearg], "w");
        if (!out)
        {
            printf("Couldn't open %s\n", argv[filearg]);
            return 1;
        }
    }        
    
    parse_text(buffer, out);
    if (out != stdout)
        fclose(out);
    return 0;
}
