/* This program compresses the help content found in help/ to standard
 * output. Do not directly compile or use this program, instead it
 * will be automatically used by genhelp.sh */

#include <ctype.h>
#include <lz4hc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "help.h"

char *compress(const char *in, int inlen, int *rc, int *minsz, int *minlev)
{
    int maxsz = LZ4_compressBound(inlen);
    unsigned char *outbuf = malloc(maxsz);
    *minsz = 9999999;
    *minlev = 0;

    for(int i = LZ4HC_CLEVEL_MIN; i < LZ4HC_CLEVEL_MAX; ++i)
    {
        *rc = LZ4_compress_HC(help_text, outbuf, inlen, maxsz, i);
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
    *rc = LZ4_compress_HC(help_text, outbuf, inlen, maxsz, *minlev);
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
            printf("0x%02x, ", buf[i]);
        }
        printf("\n");
    }
}

int main()
{
    int inlen = strlen(help_text) + 1, len;
    int minsz, minlev;
    unsigned char *outbuf = compress(help_text, inlen, &len, &minsz, &minlev);

    printf("/* auto-generated on " __DATE__ " by genhelp.sh */\n");
    printf("/* orig %d comp %d ratio %g level %d saved %d */\n", inlen, minsz, (double)minsz / inlen, minlev, inlen - minsz);
    printf("/* DO NOT EDIT! */\n\n");
    printf("const char help_text[] = {\n");

    dump_bytes(outbuf, len);
    free(outbuf);

    printf("};\n");
    printf("const unsigned short help_text_len = %d;\n", inlen);

    return 0;
}
