/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alex Gitelman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "bdf2ajf.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>

int _font_error_code = 0;
char _font_error_msg[1024];
#ifndef VC
#define strcmpi(x,y) strcasecmp(x,y)
#endif

short win_alt_map[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 00 - 0f */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 10 - 1f */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 20 - 2f */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 30 - 3f */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 40 - 4f */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 50 - 5f */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 60 - 6f */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 70 - 7f */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 80 - 8f */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 90 - 9f */
    -1, -1, -1, -1, -1, -1, -1, -1, 0x85, -1, -1, -1, -1, -1, -1, -1, /* A0 - Af */
    -1, -1, -1, -1, -1, -1, -1, -1, 0xA5, -1, -1, -1, -1, -1, -1, -1, /* B0 - Bf */
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,    /* C0 - Cf */
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,    /* D0 - Df */
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,    /* E0 - Ef */
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,    /* F0 - Ff */
};

char* bufcat(char* buf1, char *buf2, int len1, int len2)
{
    char *newbuf = malloc(len1+len2);
    if (newbuf==0)
    {
        fprintf(stderr, "Can't allocate storage in bufcat (malloc returned 0)\n");
        return NULL;
    }
    memcpy(newbuf, buf1, len1);
    memcpy(&newbuf[len1], buf2, len2);
    return newbuf;
}

void write_short(unsigned short s, unsigned char* buf)
{
    WRITE_SHORT(s, buf);
}


void usage()
{
    printf("bdf2ajf - compile BDF font for use with Rockbox\n");
    printf("Usage: bdf2ajf -f <font> -o <outfile>\n");
    printf("Options:\n");
    printf("    -f   - input BDF file\n");
    printf("    -o   - output AJF file\n");
    printf("    -h   - print help\n");
    printf("    -t <string>  - print string as bitmap\n");
    printf("    -t2 <string>  - print string as Archos bitmap (must be the same result as -t)\n");
    exit(0);

}

int do_test1 = 0;
int do_test2 = 0;

int main(int argc, char** argv)
{
    FILE *outfd = 0;
    unsigned char outbuf[20000];
    unsigned char buf[1000];
    unsigned char test_str1[300];
    unsigned char test_str2[300];
    int height = 8, rows;
    int size = 0;
    int i, offset;
    unsigned char *p, *p1;
    int c;
    char in_file[1024];
    char out_file[1024];
    BDF *bdf = 0;
    in_file[0] = 0;
    out_file[0] = 0;

    for(i=1;i<argc;i++)
    {
        if (!strcmpi(argv[i], "-f"))
        {
            i++;
            if (i==argc) usage();
            strcpy(in_file, argv[i]);
        }
        else if (!strcmpi(argv[i], "-o"))
        {
            i++;
            if (i==argc) usage();
            strcpy(out_file, argv[i]);
        }
        else if (!strcmpi(argv[i], "-t"))
        {
            i++;
            if (i==argc) usage();
            do_test1 = 1;
            strcpy(test_str1, argv[i]);
        }
        else if (!strcmpi(argv[i], "-t2"))
        {
            i++;
            if (i==argc) usage();
            do_test2 = 1;
            strcpy(test_str2, argv[i]);
        }
        else
            usage();
    }
    if (strlen(in_file)==0 || strlen(out_file)==0)
        usage();
    
    bdf = readFont(in_file);
    if (do_test1)
    {
        unsigned char *p = test_str1;
        printf("Begin Test Print1 for %s", test_str1);
        while(p[0])
            test_print(p++[0], bdf, win_alt_map);
        
    }
    if (do_test2)
    {
        unsigned char *p = test_str2;
        printf("Begin Test Print2 for %s", test_str2);
        while(p[0])
        {
            BDF_GLYPH *g = getGlyph(p++[0], bdf, win_alt_map);
            int rows = (g->bbx_height-1)/8+1;

            unsigned char tst_src[100];
            if (!g)
            {
                printf("No Glyph for %c", p[-1]);
                continue;
            }
            getBitmap(g, tst_src);
            test_print2(tst_src, g->bbx_height, g->bitmap_len*rows);
        }
        
    }
    
    writeAJF(bdf, out_file);
    return 0;
}


void writeAJF(BDF* bdf, const char* out_file)
{
    FILE *outfd = 0;
    unsigned char outbuf[20000];
    unsigned char char_map[20000];
    unsigned char buf[1000];
    unsigned char test_str1[300];
    unsigned char test_str2[300];
    unsigned short height = 8, rows, width;
    unsigned short first_char = 0;
    unsigned short char_count = 255;
    int size = 0;
    int i, offset;
    unsigned char *p, *p1;
    int c, idx, chars_offset;

    memset(outbuf,0, sizeof(outbuf));
    height = bdf->bound_height;
    rows = (bdf->bound_height-1)/8 + 1;
    width = bdf->bound_width;
    
    idx = 0;
    outbuf[idx++] = MAGIC1;
    outbuf[idx++] = MAGIC2;
    if (bdf->name)
    {
        int len = strlen(bdf->name);
        if (len<FONT_NAME_LEN)
            strcpy(&outbuf[FONT_NAME_OFFSET], bdf->name);
        else
            memcpy(&outbuf[FONT_NAME_OFFSET], bdf->name, FONT_NAME_LEN);

    }

    idx = MAX_WIDTH_OFFSET;
    write_short(width, &outbuf[idx]);
/*	outbuf[idx] = (unsigned short)width; */
    idx = HEIGHT_OFFSET;
    write_short(height, &outbuf[idx]);
/*    outbuf[idx] = (unsigned short)height; */

    idx = FIRST_CHAR_OFFSET; 
    write_short(first_char, &outbuf[idx]);

    idx = SIZE_OFFSET; 
    write_short(char_count, &outbuf[idx]);

    chars_offset = LOOKUP_MAP_OFFSET + char_count*3;
    idx = chars_offset;

    /* Put bitmaps in outbuf */
    for (c=first_char;c<=char_count;c++)
    {
        int map_offset = LOOKUP_MAP_OFFSET + (c-first_char)*3; /* index in char map. Takes 3 bytes - 1 width, 2 - offset */
        unsigned char bmp_bytes = 0;
        BDF_GLYPH *glyph = getGlyph((unsigned int)c, bdf, win_alt_map);

        if (glyph)
        {
            bmp_bytes = rows*glyph->dwidth_x;
            getBitmap(glyph, &outbuf[idx]);
        }
        else
        {
            bmp_bytes = 0;
        }

        outbuf[map_offset++] = bmp_bytes;
        write_short((unsigned short)(idx-chars_offset), &outbuf[map_offset]);
        idx+=bmp_bytes;
    }

    outfd = fopen(out_file,"wb");
    if (!outfd)
    {
        fprintf(stderr, "Can't create output file\n");
    }

    i = 0;
    for (p1=outbuf; p1 < &outbuf[idx]; p1++)
    {
        if (i==sizeof(buf))
        {
            fwrite(buf, sizeof(unsigned char), sizeof(buf), outfd);
            i=0;
        }
    
        i++;
        buf[i-1] = p1[0];
    }
    if (i>0)
        fwrite(buf, sizeof(unsigned char), i, outfd);

    fclose(outfd);
}

void freeGlyph(BDF_GLYPH* glyph)
{
    if ( glyph->glyph_name )
        free( glyph->glyph_name );

    if ( glyph->bitmap )
        free( glyph->bitmap );

    free(glyph);
}

void freeBDF(BDF* bdf)
{
    int i;
    if ( bdf->bdf_ver )
        free( bdf->bdf_ver );
    if ( bdf->name )
        free( bdf->name );

    for (i=0; i<bdf->prop_count; i++)
    {
        if ( bdf->prop_name && bdf->prop_name[i] )
            free( bdf->prop_name[i] );
        if ( bdf->prop_value && bdf->prop_value[i] )
            free( bdf->prop_value[i] );
    }
    if ( bdf->prop_name )
        free( bdf->prop_name );
    if ( bdf->prop_name )
        free( bdf->prop_value );
    for (i=0; i<bdf->char_count; i++)
    {
        if ( bdf->glyph && bdf->glyph[i] )
            freeGlyph( bdf->glyph[i] );
    }
    if ( bdf->glyph )
        free( bdf->glyph );
    free( bdf );
}

char *readLine(char *buf, char* line)
{
    while (buf[0]=='\n' || buf[0]=='\r')
        buf++;
    while (buf[0]!='\n' && buf[0]!='\r' && (buf[0]))
    {
        line++[0] = buf++[0];
    }
    line[0] = 0;
    return buf;
}

char* readNextToken()
{
    char *ret;
    char *tok = strtok(0, " ");
    if (!tok)
    {
        fprintf(stderr, "Next token is NULL\n");
        return NULL;
    }

    ret = malloc(strlen(tok)+1);
    if (!ret)
    {
        fprintf(stderr, "malloc returned 0 in readNextToken\n");
        return NULL;
    }
    strcpy(ret, tok);
    return ret;
}


int readNextIntToken()
{
    int ret;
    char* tok = readNextToken();
    if (!tok)
    {                
        fprintf(stderr, "Could not int token\n");
        return 0;
    }
    ret = atoi(tok);
    free(tok);
    return ret;
}

unsigned char getHexDigit(char c)
{
    if (c<='F' && c>='A')
        return c-'A'+10;
    if (c<='f' && c>='a')
        return c-'a'+10;
    if (c<='9' && c>='0')
        return c-'0';
    return 0;
}

void toHex(char *str, unsigned char *out)
{
    int len = strlen(str);
    out[0] = (unsigned char)16*getHexDigit(str[0]) + getHexDigit(str[1]);
    out[1] = 0;
    
    if (str[2] && len>2)
        out[1] += (unsigned char)16*getHexDigit(str[2]);
    if (str[3] && len>3)
        out[1] +=  (unsigned char)getHexDigit(str[3]);
    
}

BDF* parseBDF(char *bdf)
{
    char *p = bdf;
    char line[255];
    char *tok = NULL;
    int i=0;
    int reading_font = 0;
    int reading_properties = 0;
    int reading_glyph = 0;
    int reading_bitmap = 0;
    int current_glyph_idx = 0;
    int current_property = 0;
    unsigned char bitmap[255];
    short bitmap_idx = 0;

    BDF *ret = malloc(sizeof(BDF));
    if (!ret)
        return NULL;

    memset(ret,0,sizeof(BDF));
    
    while(p[0])
    {
        p = readLine(p, line);
        tok = strtok(line, " ");
        if (tok)
        {
            if (strcmp(tok, COMMENT)==0)
                continue;

            if (strcmp(tok, STARTFONT)==0)
            {
                if (reading_font)
                {
                    fprintf(stderr, "Only one STARTFONT allowed\n");
                    freeBDF(ret);
                    return NULL;
                }
                reading_font = 1;
                ret->bdf_ver = readNextToken();
                if (ret->bdf_ver==0)
                {                
                    fprintf(stderr, "Could not read version of BDF\n");
                    freeBDF(ret);
                    return NULL;
                }
                /* Ignore the rest */
            }
            else
            {
                if (!reading_font)
                {
                    fprintf(stderr, "Font must start with STARTFONT\n");
                    freeBDF(ret);
                    return NULL;
                }
                
                if (!strcmp(tok, ENDFONT))
                {
                    break;
                }
                else if (!strcmp(tok, FONT))
                {
                    ret->name = readNextToken();
                    if (ret->name==0)
                    {                
                        fprintf(stderr, "Could not read name font\n");
                        freeBDF(ret);
                        return NULL;
                    }
                }
                else if (!strcmp(tok, SIZE))
                {
                    ret->point_size = readNextIntToken();
                    if (ret->point_size<=0)
                    {
                        fprintf(stderr, "Font point size is invalid\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    
                    ret->x_res = readNextIntToken();
                    if (ret->x_res<=0)
                    {
                        fprintf(stderr, "Device x resolution is invalid\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    ret->y_res = readNextIntToken();
                    if (ret->y_res<=0)
                    {
                        fprintf(stderr, "Device y resolution is invalid\n");
                        freeBDF(ret);
                        return NULL;
                    }
                }
                else if (!strcmp(tok, FONTBOUNDINGBOX))
                {
                    ret->bound_width = readNextIntToken();
                    if (ret->bound_width<=0)
                    {
                        fprintf(stderr, "Bounding width is invalid\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    ret->bound_height = readNextIntToken();
                    if (ret->bound_height<=0)
                    {
                        fprintf(stderr, "Bounding height is invalid\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    ret->bound_disp_x = readNextIntToken();
                    ret->bound_disp_y = readNextIntToken();
                }
                else if (!strcmp(tok, STARTPROPERTIES))
                {
                    ret->prop_count = readNextIntToken();
                    if (ret->prop_count<=0)
                    {
                        fprintf(stderr, "Number of properties must be > 0\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    ret->prop_name = malloc(ret->prop_count*sizeof(char*));
                    if (ret->prop_name == 0)
                    {
                        fprintf(stderr, "Can't allocate storage for properties (malloc returns 0)\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    memset(ret->prop_name, 0, ret->prop_count*sizeof(char*));

                    ret->prop_value = malloc(ret->prop_count*sizeof(char*));
                    if (ret->prop_value==0)
                    {
                        fprintf(stderr, "Can't allocate storage for properties (malloc returns 0)\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    memset(ret->prop_value, 0, ret->prop_count*sizeof(char*));
                    current_property = 0;
                    reading_properties = 1;
                }
                else if (!strcmp(tok, ENDPROPERTIES))
                {
                    if (!reading_properties)
                    {
                        fprintf(stderr, "ENDPROPERTIES found while not reading properties\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    if (current_property!=ret->prop_count)
                    {
                        fprintf(stderr, "Property count mismatch\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    reading_properties = 0;
                }
                else if (!strcmp(tok, CHARS))
                {
                    ret->char_count = readNextIntToken();
                    if (ret->char_count<=0)
                    {
                        fprintf(stderr, "Font must have >0 char glyphs\n");
                        freeBDF(ret);
                        return NULL;
                    }

                    ret->glyph = malloc(ret->char_count*sizeof(BDF_GLYPH*));
                    if (!ret->glyph)
                    {
                        fprintf(stderr, "Can't allocate storage for glyphs (malloc returns 0)\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    memset(ret->glyph, 0, ret->char_count*sizeof(BDF_GLYPH*));
                }
                else if (!strcmp(tok, STARTCHAR))
                {
                    if (reading_glyph)
                    {
                        fprintf(stderr, "Nested STARTCHAR is not allowed\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    if (current_glyph_idx>=ret->char_count)
                    {
                        fprintf(stderr, "Too many glyphs (more than defined by CHARS)\n");
                        freeBDF(ret);
                        return NULL;
                    }

                    reading_glyph = 1;
                    ret->glyph[current_glyph_idx] = malloc(sizeof(BDF_GLYPH));
                    if (ret->glyph[current_glyph_idx]==0)
                    {
                        fprintf(stderr, "Can't allocate storage for one glyph (malloc returns 0)\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    memset(ret->glyph[current_glyph_idx],0, sizeof(BDF_GLYPH));
                    ret->glyph[current_glyph_idx]->glyph_name = readNextToken();

                    memset(bitmap, 0, sizeof(bitmap));
                    bitmap_idx = 0;

                }
                else if (!strcmp(tok, ENDCHAR))
                {
                    if (!reading_glyph)
                    {
                        fprintf(stderr, "ENDCHAR only allowed after STARTCHAR\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    reading_glyph = 0;
                    reading_bitmap = 0;
                    ret->glyph[current_glyph_idx]->bitmap_len = bitmap_idx/2;
                    ret->glyph[current_glyph_idx]->bitmap = malloc(bitmap_idx);
                    if (ret->glyph[current_glyph_idx]->bitmap==0)
                    {
                        fprintf(stderr, "Can't allocate storage for bitmap (malloc returns 0)\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    
                    for(i=0;i<bitmap_idx;i+=2)
                    {
                        ret->glyph[current_glyph_idx]->bitmap[i] = bitmap[i];
                        ret->glyph[current_glyph_idx]->bitmap[i+1] = bitmap[i+1];
                    }
                    if (ret->glyph[current_glyph_idx] ==0)
                                _font_error_code = 3;

                    if (ret->glyph[current_glyph_idx]->encoding>=0)
                    {
                        ret->enc_table[ret->glyph[current_glyph_idx]->encoding] = ret->glyph[current_glyph_idx];
                    }
                    
                    current_glyph_idx++;
                }
                else if (!strcmp(tok, ENCODING))
                {
                    if (!reading_glyph)
                    {
                        fprintf(stderr, "ENCODING only allowed after STARTCHAR\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    ret->glyph[current_glyph_idx]->encoding = readNextIntToken();
                    /* TODO: Consider encoding ? */
                }
                else if (!strcmp(tok, SWIDTH))
                {
                    if (!reading_glyph)
                    {
                        fprintf(stderr, "SWIDTH only allowed after STARTCHAR\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    ret->glyph[current_glyph_idx]->swidth_x = readNextIntToken();
                    ret->glyph[current_glyph_idx]->swidth_y = readNextIntToken();
                }
                else if (!strcmp(tok, DWIDTH))
                {
                    if (!reading_glyph)
                    {
                        fprintf(stderr, "DWIDTH only allowed after STARTCHAR\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    ret->glyph[current_glyph_idx]->dwidth_x = readNextIntToken();
                    ret->glyph[current_glyph_idx]->dwidth_y = readNextIntToken();
                }
                else if (!strcmp(tok, BBX))
                {
                    if (!reading_glyph)
                    {
                        fprintf(stderr, "BBX only allowed after STARTCHAR\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    ret->glyph[current_glyph_idx]->bbx_width = readNextIntToken();
                    ret->glyph[current_glyph_idx]->bbx_height = readNextIntToken();
                    ret->glyph[current_glyph_idx]->bbx_disp_x = readNextIntToken();
                    ret->glyph[current_glyph_idx]->bbx_disp_y = readNextIntToken();
                }
                else if (!strcmp(tok, BITMAP))
                {
                    if (!reading_glyph)
                    {
                        fprintf(stderr, "BITMAP only allowed after STARTCHAR\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    reading_bitmap = 1;

                }
                else /* Last one reading properties and bitmaps */
                {
                    if (reading_glyph && reading_bitmap)
                    {
                        toHex(tok, &bitmap[bitmap_idx]);
                        bitmap_idx+=2;
                        continue;
                    }


                    if (!reading_properties)
                    {
                        fprintf(stderr, "Unknown token %s", tok);
                        continue;
                    }
                    if (current_property>=ret->prop_count)
                    {
                        fprintf(stderr, "Too many properties\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    ret->prop_name[current_property] = malloc(strlen(tok)+1);
                    if (ret->prop_name[current_property]==0)
                    {
                        fprintf(stderr, "Can't allocate storage for one property name (malloc returned 0)\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    strcpy(ret->prop_name[current_property], tok);
                    ret->prop_value[current_property] = readNextToken();
                    if (ret->prop_value[current_property]==0)
                    {
                        fprintf(stderr, "Can't allocate storage for one property value (malloc returned 0)\n");
                        freeBDF(ret);
                        return NULL;
                    }
                    current_property++;
                }

            }
        }

    }

    return ret;
}



BDF* readFont(const char *name)
{
    char buf[1001];
    int count;
    char *font_buf = 0;
    char *new_buf;
    int len = 0;

    FILE *fd = fopen(name, "rt");
    if (!fd)
    {
        fprintf(stderr, "%d %s. ", errno, name);
        return NULL;
    }

    count = fread(buf, sizeof(char), 1000, fd);

    new_buf = malloc(count);
    if (!new_buf)
    {
        fprintf(stderr, "mlc=0. len %d.", count);
        fclose(fd);
        return NULL;
    }

    memcpy(new_buf, buf, count);
    len = count;
    font_buf = new_buf;
    while (count>0)
    {
        char *p;
        count = fread( buf, sizeof(char), sizeof(buf)-1, fd);
        if (count<=0)
            break;
        p = new_buf;
        new_buf = bufcat(new_buf, buf, len, count);
        if (!new_buf)
        {
            fclose(fd);
            return NULL;
        }
        free(p);
        len+=count;
        font_buf = new_buf;
    }
    
    close(fd);
    font_buf[len-1] =0;
    return parseBDF(font_buf);
}



void test_print(unsigned char c, BDF* font, short* enc_map)
{
    int i,j;
    BDF_GLYPH *g;

    if (enc_map && enc_map[c]>0)
        c = enc_map[c];
    g = font->enc_table[c];
    if (!g)
        g = font->glyph[c];
    if (!g)
    {
        fprintf(stderr,"Missing %d \n",c);
        return;
    }
    else
        printf("%s %d enc: %d %x \n", 
               g->glyph_name,c, c, g->encoding, g->encoding); 
    for (i=0; i < g->bitmap_len; i++)
    {
        unsigned short bmp = 0;
        unsigned short sh;
        DOUBLE_BYTE db;
        db.db[0] = g->bitmap[i*2];
        db.db[1] = g->bitmap[i*2+1];
        
        sh = 1 << 7; /*g->dwidth_x;*/

        for(j=0; j < g->dwidth_x; j++)
        {
            int b;
            unsigned short bit = sh>>j;
            if (bit==0)
            {
                sh = 1 << (sizeof(unsigned short)*8-1);
                bit = sh>>(j - 8);
            }
            b = bit & db.sval;
            printf( b ? "*" : " " );
        }
        
        printf("\n");
    }


}

BDF_GLYPH* getGlyph(unsigned char c, BDF* bdf, short* enc_map)
{
    BDF_GLYPH* ret;
    if (enc_map && enc_map[c]>0)
        c = enc_map[c];

    ret = bdf->enc_table[c];
    if (!ret && c < bdf->char_count)
        ret = bdf->glyph[c];

    if (!ret)
        fprintf(stderr, "Glyph %d is 0.\n", c);

    return ret;
}

void test_print2(unsigned char *src, int height, int len)
{
    int rows = (height-1)/8+1;
    int r,c, bit;
    int rp = 0;
    int i;
    printf("Bitmap: ");
    for (i=0;i<len;i++)
        printf("0x%x ", src[i]);
    printf("\n");

    for (r=0; r < rows; r++)
    {
        for (bit=0; bit < 8; bit++)
        {
            int sh = 1 << bit;
            for (c=0; c < len/rows; c++)
            {
                int tst = src[c*rows+r]&sh;
                if (tst)
                    printf("*");
                else
                    printf(" ");
            }
            printf("\n");
            if (rp++ > height)
                return;
        }
    }
    
}

void getBitmap(BDF_GLYPH* g, unsigned char* src)
{
    int i,j,k;
    int d = 0;
    int map_shift = 0;
    int rows = (g->bbx_height-1)/8+1;
    memset(src, 0, g->bitmap_len*rows);
    
    for (i=0; i < g->bitmap_len; i++)
    {
        unsigned short bmp = 0;
        unsigned short sh, srcmap;
        DOUBLE_BYTE db;
        db.db[0] = g->bitmap[i*2];
        db.db[1] = g->bitmap[i*2+1];
        
        sh = 1 << 7; /*g->dwidth_x;*/

        if (i>0 && (i%8==0))
        {
            d++;
            map_shift = 0;
        }
        srcmap = 1<<map_shift++;

        for(j=0; j < g->dwidth_x; j++)
        {
            int b;
            unsigned short bit = sh>>j;
            if (bit==0)
            {
                sh = 1 << (sizeof(unsigned short)*8-1);
                bit = sh>>(j - 8);
            }
            b = bit&db.sval;

            if (b)
                src[j*rows+d] |= srcmap;
        }
    }
}

