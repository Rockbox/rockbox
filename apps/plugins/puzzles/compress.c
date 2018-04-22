/* This program compresses the help content found in help/ to standard
 * output. Do not directly compile or use this program, instead it
 * will be automatically used by genhelp.sh */

#include <assert.h>
#include <ctype.h>
#include <lz4hc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "help.h"

char *compress(const char *in, int inlen, int *rc, int *minsz, int *minlev)
{
    int maxsz = LZ4_compressBound(inlen);
    unsigned char *outbuf = malloc(maxsz);
    *minsz = 9999999;
    *minlev = 0;

    for(int i = LZ4HC_CLEVEL_MIN; i < LZ4HC_CLEVEL_MAX; ++i)
    {
        *rc = LZ4_compress_HC(in, outbuf, inlen, maxsz, i);
        if(!*rc)
        {
            fprintf(stderr, "compress failed\n");
            return NULL;
        }
        if(*rc < *minsz)
        {
            *minsz = *rc;
            *minlev = i;
        }
    }
    *rc = LZ4_compress_HC(in, outbuf, inlen, maxsz, *minlev);
    return outbuf;
}

void dump_bytes(unsigned char *buf, int len)
{
    int i = 0;
    while(i < len)
    {
        int stop = i + 10;
        for(;i < stop && i < len; ++i)
        {
            unsigned char c = buf[i];
            printf("0x%02x,", c);
            if(i != stop - 1 && i != len - 1)
                printf(" ");
        }
        printf("\n");
    }
}

/* Input: help text in help_text array (to be compiled against) as a
 * standard C string
 * Output: C source code on stdout defining the following:
 *
 * const char help_text_words[];
 * const unsigned short help_text_len;
 * struct style_text help_text_style[];
 *
 * help_text_words consists of help_text_len bytes containing the
 * words of the help text, delimited with NULs, not a standard C
 * string. The rockbox frontend is responsible for generating an array
 * of pointers into this array to pass to display_text. */

int main()
{
    int inlen = strlen(help_text) + 1, outlen;
    int minsz, minlev;

    printf("/* auto-generated on " __DATE__ " by genhelp.sh */\n");
    printf("/* help text is compressed using LZ4; see compress.c for details */\n");
    printf("/* DO NOT EDIT! */\n\n");

    printf("#include \"lib/display_text.h\"\n\n");

    printf("struct style_text help_text_style[] = {\n");

    /* break up words on spaces and newline while printing indices of
     * underlined words */
    char *buf = strdup(help_text);
    bool underline = false, center = false;
    int word_idx = 0, help_text_len = inlen;

    for(int i = 0; i < help_text_len; ++i)
    {
        switch(buf[i])
        {
        case '#':
        case '_':
            /* title or underlined portion */
            if(buf[i] == '#')
            {
                center = !center;
                if(center)
                    printf(" { %d, TEXT_CENTER | C_RED },\n", word_idx);
            }
            else
            {
                /* underline */
                underline = !underline;

                if(underline)
                    printf(" { %d, TEXT_UNDERLINE },\n", word_idx);
            }

            /* delete the formatting character */
            memmove(buf + i, buf + i + 1, help_text_len - i - 1);
            --help_text_len;
            --i;
            break;
        case '$':
            /* genhelp.sh replaces the underscores in URLs with
             * dollar signs to help us out. */
            buf[i] = '_';
            break;
        case '\n':
            center = false;
            /* fall through */
        case ' ':
        case '\0':
            /* Groups of words that are centered or underlined are
             * counted as a single "word". */
            if(!underline && !center)
            {
                buf[i] = '\0';
                ++word_idx;
            }
            break;
        }
    }
    /* sanity check */
    int words_check = 0;
    for(int i = 0; i < help_text_len; ++i)
        if(!buf[i])
            ++words_check;

    assert(words_check == word_idx);

    printf(" LAST_STYLE_ITEM\n};\n\n");

    /* remove trailing NULs */
    for(int i = help_text_len - 2; i >= 0 && !buf[i]; --i, --help_text_len, --word_idx);

    unsigned char *outbuf = compress(buf, help_text_len, &outlen, &minsz, &minlev);

    printf("/* orig %d comp %d ratio %g level %d saved %d */\n", help_text_len, minsz, (double)minsz / help_text_len, minlev, help_text_len - minsz);
    printf("const char help_text[] = {\n");

    dump_bytes(outbuf, outlen);
    free(outbuf);
    free(buf);

    printf("};\n\n");
    printf("const unsigned short help_text_len = %d;\n", help_text_len);
    printf("const unsigned short help_text_words = %d;\n", word_idx);

    return 0;
}
