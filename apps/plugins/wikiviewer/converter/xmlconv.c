/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Matthias Larisch
 * Copyright (C) 2007 Adam Gashlin (hcs)
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

/*
   Usage: xmlconv wiki_xml output_prefix

   generates output_prefix.wwr, output_prefix.wwt, output_prefixN.wwa where N is
   0-? (each file ~ 1 GB)

   This is a rewrite from original ruby converter (2007 by Frederik Vestre)
 */

#include <stdio.h>
#include <pcre.h>
#include <limits.h>
#include <string.h>
#include <zlib.h>
#include <ctype.h>
#include "xmlconv.h"
#include "xmlentities.h"

static FILE *titlefile;
static FILE *redirectfile;
static FILE *outfile;
static int outfilenum;
static char outfilename[PATH_MAX];
static pcre *regex_redirect;
static int ovector_temp[30];
static int matchcount_temp;

static article_header buf_article_header;
static title_entry buf_title_entry;

static void strreverse(char* begin, char* end)
{
    char aux;
    while(end>begin)
    {
        aux=*end;
        *end--=*begin;
        *begin++=aux;
    }
}

static void itoa(int value, char* str, int base)
{
    static char num[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char* wstr=str;
    int sign;

    /* Validate base */
    if (base<2 || base>35)
    {
        *wstr='\0'; return;
    }

    /* Take care of sign */
    if ((sign=value) < 0) value = -value;

    /* Conversion. Number is reversed. */
    do {
        *wstr++ = num[value%base];
    } while(value/=base);

    if(sign<0)
        *wstr++='-';

    *wstr='\0';

    /* Reverse string */
    strreverse(str,wstr-1);
}

static int open_outfile(int mode)
{
    char filename[PATH_MAX];
    strcpy(filename,outfilename);
    if(mode==1) outfilenum++;

    itoa(outfilenum,filename+strlen(filename),10);
    strcat(filename,".wwa");

    if(mode==1)
    {
        if((outfile=fopen(filename,"w"))==NULL)
        {
            printf("Error on creating article output file\n");
            return 0;
        }
    }
    else if((outfile=fopen(filename,"a"))==NULL)
    {
        printf("Error on reopening article output file\n");
        return 0;
    }

    return 1;
}

static void insert_redirect(char redirect_from[255], char redirect_to[255])
{
    unsigned int buffer;
    buffer=strlen(redirect_from);
    fwrite(&buffer, sizeof(unsigned int), 1, redirectfile);
    buffer=strlen(redirect_to);
    fwrite(&buffer, sizeof(unsigned int), 1, redirectfile);
    fputs(redirect_from,redirectfile);
    fputs(redirect_to,redirectfile);
}

/*
   write a unicode value to a character buffer as UTF-8 increments *offset by
   numer of bytes written to buf by hcs, based on the wikipedia article on UTF-8
 */
static void writeUTF8(char * buf, int * offset, int value)
{
    if (value>=0 && value<=0x7f)
        buf[(*offset)++]=value;
    else if (value>=0x80 && value <=0x7ff)
    {
        buf[(*offset)++]=((value >> 6)&0x1f)|0xC0;
        buf[(*offset)++]=(value&0x3f)|0x80;
    }
    else if (value>=0x800 && value <=0xffff)
    {
        buf[(*offset)++]=((value >> 12)&0xf)|0xe0;
        buf[(*offset)++]=((value >> 6)&0x3f)|0x80;
        buf[(*offset)++]=(value&0x3f)|0x80;
    }
    else if (value>=0x10000 && value <= 0x10ffff)
    {
        buf[(*offset)++]=((value >> 18)&0xf)|0xf0;
        buf[(*offset)++]=((value >> 12)&0x3f)|0x80;
        buf[(*offset)++]=((value >> 6)&0x3f)|0x80;
        buf[(*offset)++]=(value&0x3f)|0x80;
    }
    else
        perror("writeUTF8");
}

/* wiki code and HTML parsing (by hcs) */

static int inbold;
static int initalic;

/*
   int parse_(char * outbuf, const char * inbuf, int * const inlen);
   outbuf: output buffer inbuf: input buffer inlength: total length of input
   buffer (few parsers will consume the entire input), decremented by number of
   bytes consumed
 */

#define CONSUME(amt) { const int tempc=(amt); inbuf+=tempc; inlen-=tempc; }
#define PRODUCE(c) { (*(outbuf++))=(c); }
#define PRODUCEUTF8(c) { int temp=0; writeUTF8(outbuf, &temp, (c)); outbuf+=temp; }
/* between PARSEHEAD and PARSETAIL goes a block for the body of the while loop
 */
#define PARSEHEAD(name) static void doparse_ ## name(char ** outbufptr, const char ** inbufptr, int * const inlenptr) { \
        const char * inbuf = *inbufptr; \
        char * outbuf = *outbufptr; \
        int inlen = *inlenptr; \
        while (inlen>0)
#define PARSETAIL \
    *inbufptr = inbuf; \
    *outbufptr = outbuf; \
    *inlenptr = inlen; \
    }
#define DESCEND(name) doparse_ ## name(&outbuf,&inbuf,&inlen)
#define PASS { PRODUCE(inbuf[0]); CONSUME(1); }
#define PARSEPROTO(name) static void doparse_ ## name(char ** outbufptr, const char ** inbufptr, int * const inlenptr)
#define PARSEPROTOINLINE(name) static inline void doparse_ ## name(char ** outbufptr, const char ** inbufptr, int * const inlenptr)

/* there has got to be a better way */

PARSEPROTO(comment);
PARSEPROTO(linkname);
PARSEPROTO(link);
PARSEPROTO(ref);
PARSEPROTO(article);
PARSEPROTOINLINE(text);

/* assume we're just past the comment opening tag */
PARSEHEAD(comment)
{
    if (inbuf[0]=='-' && !memcmp(inbuf+1,"-&gt;",5))
    {
        CONSUME(6);
        break;
    }
    else
        CONSUME(1);
}
PARSETAIL

PARSEHEAD(linkname)
{
    if (inbuf[0]=='|')
    {
        CONSUME(1);
        /* take back anything we've written so far, only the last is display
           text */
        outbuf=*outbufptr;
    }
    else if (inbuf[0]==']' && inbuf[1]==']')
        break;
    else if (inbuf[0]=='[' && inbuf[1]=='[')
    {
        /* for instance, links in image caption */
        PRODUCE(MARKUP_STARTLINK);
        CONSUME(2);
        DESCEND(link);
    }
    else DESCEND(text);
}
PARSETAIL

PARSEHEAD(link)
{
    if (inbuf[0]==']' && inbuf[1]==']')
    {
        CONSUME(2);
        PRODUCE(MARKUP_ENDLINK);
        break;
    }
    else if (inbuf[0]=='|')
    {
        CONSUME(1);
        PRODUCE(MARKUP_MODE);
        DESCEND(linkname);
        /* should come back with ]] */
        CONSUME(2);
        PRODUCE(MARKUP_ENDLINK);
        break;
    }
    else DESCEND(text);
}
PARSETAIL

PARSEHEAD(ref)
{
    if (inbuf[0]=='&' && !memcmp(inbuf+1,"lt;/",4) && tolower(inbuf[5])=='r' &&
        tolower(inbuf[6])=='e' && tolower(inbuf[7])=='f' &&
        !memcmp(inbuf+8,"&gt;",4))
    {
        CONSUME(12);
        break;
    }
    else
        CONSUME(1);
}
PARSETAIL

PARSEHEAD(article)
{
    /* links */
    switch (inbuf[0])
    {
    case '[':
        if (!memcmp(inbuf,"[[",2))
        {
            /* better-than-nothing image processing */
            if (!memcmp(inbuf+3,"mage:",5) && (inbuf[2]=='i' || inbuf[2]=='I'))
            {
                CONSUME(2);
                PRODUCE(MARKUP_STARTLINK);
                while (inbuf[0]!='|' && memcmp(inbuf,"]]",2) )
                {
                    PASS;
                }
                PRODUCE(MARKUP_MODE);
                PRODUCE('[');
                PRODUCE('I');
                PRODUCE(']');
                PRODUCE(MARKUP_ENDLINK);
                PRODUCE(' ');
                if (!memcmp(inbuf,"]]",2))
                {
                    CONSUME(2);
                }
                else
                {
                    CONSUME(1);
                    DESCEND(linkname);
                    /* should come back with ]] */
                    CONSUME(2);
                    PRODUCE('\n');
                }
            }
            else
            {
                PRODUCE(MARKUP_STARTLINK);
                CONSUME(2);
                DESCEND(link);
            }

            break;
        }

    default:
        DESCEND(text);
    }
}
PARSETAIL

static int entity_comparer(const void *_key, const void *_ent)
{
    const char* key = (const char*) _key;
    struct html_entity* ent = (struct html_entity*) _ent;

    return strncmp(key, ent->string, ent->string_length);
}

/* stuff that can be anywhere in plain text */
/* only processes current character (unless it is something we recognize) */

static inline void doparse_text(char ** outbufptr, const char ** inbufptr, int * const inlenptr)
{
    const char * inbuf = *inbufptr;
    char * outbuf = *outbufptr;
    int inlen = *inlenptr;

    switch (inbuf[0])
    {
    case '\0':
        printf("Read zero but end of string not reached\n");
        CONSUME(1);
        break;
    case '\'':
        if (!memcmp(inbuf+1,"''",2))
        {
            CONSUME(3);
            if (inbold)
            {
                PRODUCE(MARKUP_ENDBOLD);
                inbold=0;
            }
            else
            {
                PRODUCE(MARKUP_STARTBOLD);
                inbold=1;
            }

            break;
        }

        if (inbuf[1]=='\'')
        {
            CONSUME(2);
            if (initalic)
            {
                PRODUCE(MARKUP_ENDITALIC);
                initalic=0;
            }
            else
            {
                PRODUCE(MARKUP_STARTITALIC);
                initalic=1;
            }

            break;
        }

        PASS;
        break;
    case '&':
        /* comment */
        if (!memcmp(inbuf+1,"lt;!--",6))
        {
            CONSUME(7);
            DESCEND(comment);
            break;
        }

        /* references (really clutter things up) */
        if (!memcmp(inbuf+1,"lt;",3) && tolower(inbuf[4])=='r' &&
            tolower(inbuf[5])=='e' && tolower(inbuf[6])=='f' &&
            (inbuf[7]=='/' || inbuf[7]==' ' || inbuf[7]=='&'))
        {
            int i;
            CONSUME(7);

            /* handle <ref /> (stanislaw lem) */
            /* find end angle bracket */
            for (i=0;; i++)
            {
                if (inbuf[i]=='&' && !memcmp(inbuf+i+1,"gt;",3))
                    break;
            }
            /* check for self-terminating tag */
            if (inbuf[i-1]=='/')
            {
                CONSUME(i+4);
            }
            else
            {
                DESCEND(ref);
            }

            break;
        }

        /* entities */
        /* &amp; &gt; &lt; and &quot; are necessary for storage in XML */
        if (!memcmp(inbuf+1,"amp;",4))
        {
            /*
               might contain another entitized entity (&ndash; appears in the
               dump as &amp;ndash;)
             */

            CONSUME(5);

            if (inbuf[0] == '#') /* numeric (thus far untested) */
            {
                int consumed=0;
                int value=0;
                char d;
                consumed++;
                if (inbuf[consumed]=='x')
                { /* hexadecimal */
                    for (consumed++; isxdigit(d=inbuf[consumed]); consumed++)
                    {
                        if (isupper(d)) value=value*16+d-'A'+10;
                        else if (islower(d)) value=value*16+d-'a'+10;
                        else if (isdigit(d)) value=value*16+d-'0';
                        else break;  /* this condition should be redundant */
                    }
                    /* I do not consider &#x; to be a valid entity */
                    if (consumed > 3 && inbuf[consumed]==';')
                    {
#ifdef DEBUG
                        {
                            int i;
                            for (i=0; i<6 && inbuf[i]!=';' && inbuf[i]!='&'; i++) ;
                            if (inbuf[i]==';')
                            {
                                printf("numeric entity: ");
                                for (i=0; inbuf[i]!=';'; i++)
                                {
                                    printf("%c",inbuf[i]);
                                }
                                printf(" evaluated as 0x%x\n",value);
                            }
                        }
#endif
                        consumed++;
                        PRODUCEUTF8(value);
                        CONSUME(consumed);
                        break;
                    }

                    /* otherwise fall through to normal handling */
                }
                else  /* decimal */
                {
                    for (; isdigit(d=inbuf[consumed]); consumed++)
                    {
                        if (isdigit(d)) value=value*10+d-'0';
                        else break;  /* this condition should be redundant */
                    }
                    /* I do not consider &#; to be a valid entity */
                    if (consumed > 2 && inbuf[consumed]==';')
                    {
#ifdef DEBUG
                        {
                            int i;
                            for (i=0; i<6 && inbuf[i]!=';' && inbuf[i]!='&'; i++) ;
                            if (inbuf[i]==';')
                            {
                                printf("numeric entity: ");
                                for (i=0; inbuf[i]!=';'; i++)
                                {
                                    printf("%c",inbuf[i]);
                                }
                                printf(" evaluated as %d\n",value);
                            }
                        }
#endif
                        consumed++;
                        PRODUCEUTF8(value);
                        CONSUME(consumed);
                    }

                    /* otherwise fall through to normal handling */
                }
            }

            struct html_entity* result = bsearch(inbuf, entities, ENT_COUNT,
                                                 sizeof(struct html_entity),
                                                 &entity_comparer);
            if(result != NULL)
            {
                PRODUCEUTF8(result->utf8);
                CONSUME(result->string_length);
                break;
            }

            {
                int i;
                for (i=0; i<6 && inbuf[i]!=';' && (isalpha(inbuf[i]) || isdigit(inbuf[i])); i++) ;
                if (inbuf[i]==';')
                {
                    printf("unsupported entity: ");
                    for (i=0; inbuf[i]!=';'; i++)
                    {
                        printf("%c",inbuf[i]);
                    }
                    printf("\n");
                }
            }

            PRODUCE('&');
            break;
        }

        if (!memcmp(inbuf+1,"lt;",3))
        {
            PRODUCE('<');
            CONSUME(4);
            break;
        }

        if (!memcmp(inbuf+1,"gt;",3))
        {
            PRODUCE('>');
            CONSUME(4);
            break;
        }

        if (!memcmp(inbuf+1,"quot;",5))
        {
            PRODUCE('"');
            CONSUME(6);
            break;
        }

        {
            int i;
            printf ("unknown entity at top level: ");
            for (i=0; inbuf[i]!=';'; i++)
            {
                printf("%c",inbuf[i]);
            }
            printf("\n");
        }

        PASS;
        break;
    default:
        PASS;
    }
PARSETAIL

static int parse_article(char * article_parsed, char * article, int article_length_temp)
{
    char * article_parsed_end = article_parsed;
    const char * article_end = article;
    int templen = article_length_temp;
    inbold=0;
    initalic=0;
    doparse_article(&article_parsed_end,&article_end,&templen);

    return article_parsed_end-article_parsed;
}

/* */

static void do_article(int id, char title[255], char *article, int article_length)
{
    /* Redirect processing, redirects are short in general... speedup :) */
    if(article_length<1000)
    {
        matchcount_temp=pcre_exec(regex_redirect, NULL, article, article_length,
                                    0, PCRE_NOTEMPTY, ovector_temp, 30);
        if(matchcount_temp>1)
        {
            char redirect_to[255];
            pcre_copy_substring(article, ovector_temp, matchcount_temp, 1,
                                redirect_to, 255);
            if(strlen(redirect_to)<1)
                printf("Could not process redirect %s\n",title);
            else
            {
                insert_redirect(title, redirect_to);
                return;
            }
        }
    }

    static char article_parsed[S_ARTBUF*2];
    int article_parsed_length=0;

    /*
        This function processes the stylistics in the article. We give the
        pointer as parameter so it changes our article in same memory
        */
    article_parsed_length=parse_article(article_parsed, article, article_length);

    /*
        At this point we should have the correct article in article-string.
        This needs to be compressed using gzip and then written to the
        outfile:
        */
    buf_article_header.id=id;
    buf_article_header.article_length=article_parsed_length;
    buf_article_header.title_length=strlen(title);
    if((int)ftell(outfile)+buf_article_header.article_length >= 0x40000000)
    {
        /* Begin new output file at about 1gb */
        fclose(outfile);
        open_outfile(1);
    }

    buf_title_entry.filenumber=outfilenum;
    buf_title_entry.fileposition=(int)ftello(outfile);
    buf_title_entry.title_length=buf_article_header.title_length;
    fwrite (&buf_title_entry, sizeof(buf_title_entry), 1, titlefile);
    fwrite (title, sizeof(char), buf_title_entry.title_length, titlefile);

    gzFile *gzipfile;
    if((gzipfile=gzdopen(dup(fileno(outfile)),"w9"))==NULL)
    {
        perror("gzdopen");
        return;
    }

    gzwrite(gzipfile, &buf_article_header, sizeof(buf_article_header));
    gzwrite(gzipfile, title, buf_title_entry.title_length);
    gzwrite(gzipfile, article_parsed, article_parsed_length);
    gzflush(gzipfile, Z_FINISH);
    gzclose(gzipfile);
    fseeko(outfile,0,SEEK_END);
}

static void display_progress(long value, long max)
{
    long i, normalized_val = ((long double)value/(long double)max)*80;

    max -= 5; /* sizeof("[" "]xx%") */

    printf("[");
    for(i=0; i<80; i++)
        printf("%c", i < normalized_val ? '*' : ' ');
    printf("]%02d%%\r", (int) (((long double)value/(long double)max)*100));
    fflush(stdout);
}

int main(int argc,char * argv[])
{
    printf("Wikipedia XML Converter 0.12\n");
    if(argc < 3)
    {
        printf("Help:\nThis program converts wikipedia-xml-dump to rockbox ww format.\n");
        printf("Usage: xmlconv <input-xml-dump> <output-prefix>\n");
        printf("example: xmlconv dewiki.xml meindewiki\n");
        return 0;
    }

    FILE *xmldump;

    strcpy(outfilename,argv[2]);
    if (argv[1][0]=='-' && argv[1][1]=='\0')
    {
        printf("working on stdin with output prefix %s\n",argv[2]);
        xmldump = stdin;
    }
    else
    {
        printf("working on %s with output prefix %s\n",argv[1], outfilename);
        if((xmldump=fopen(argv[1],"rb"))==NULL)
        {
            printf("Error on opening input file\n");
            return 1;
        }
    }

    if((titlefile=fopen(strcat(outfilename,".wwt"),"wb"))==NULL)
    {
        printf("Error on opening title output file\n");
        return 1;
    }

    strcpy(outfilename,argv[2]);
    if((redirectfile=fopen(strcat(outfilename,".wwr"),"wb"))==NULL)
    {
        printf("Error on opening redirect output file\n");
        return 1;
    }

    strcpy(outfilename,argv[2]);
    outfilenum=-1;
    if(!open_outfile(1)) return 1;

    long long int dumplen=0, actlen=0;
    if (xmldump != stdin)
    {
        fseeko(xmldump,0,SEEK_END);
        dumplen=ftello(xmldump);
        fseeko(xmldump,0,SEEK_SET);
    }
    else
        dumplen=-1;

    char linebuffer[S_ROWBUF];
    int i=0;
    const char *error=NULL;
    int erroffset;
    int ovector_open[30], ovector_end[30];
    int matches_open, matches_closed;
    pcre *regex_tagname, *regex_endtag, *regex_endtag_text, *regex_betweentags;
    regex_tagname=pcre_compile("<([a-zA-Z0-9]+)[^>/]*(/)?>", 0, &error, &erroffset, NULL);
    regex_endtag=pcre_compile("</([a-zA-Z0-9]+)[^>]*>", 0, &error, &erroffset, NULL);
    regex_endtag_text=pcre_compile("([^<]*)</[a-zA-Z0-9]+[^>]*>", 0, &error, &erroffset, NULL);
    regex_betweentags=pcre_compile("<([a-zA-Z0-9]+)[^>]*>([^<]*)(</\\1[^>]*>)?", 0, &error, &erroffset, NULL);
    regex_redirect=pcre_compile("(?i)#REDIRECT:? ?\\[\\[([^\\]]*)]]", 0, &error, &erroffset, NULL);
    char tagbuf_o[30];
    char tagbuf_c[30];
    char title[255];
    int id=0;
    char article[S_ARTBUF+1];
    char buffer[255];
    int article_length=0;
    int mode=TAG_OUTSIDE;

    i=0;
    while (fgets(linebuffer,8191,xmldump)!=0)
    {
        i++;
        if(i%5000==0 && dumplen > 0)
        {
            actlen=ftello(xmldump);
            display_progress(actlen, dumplen);
        }

        linebuffer[strlen(linebuffer)-1]=0;
        matches_open=pcre_exec(regex_tagname, NULL, linebuffer, strlen(linebuffer), 0, PCRE_NOTEMPTY, ovector_open, 30);
        matches_closed=pcre_exec(regex_endtag, NULL, linebuffer, strlen(linebuffer), 0, PCRE_NOTEMPTY, ovector_end, 30);

        if(matches_open>=2)
        {
            pcre_copy_substring(linebuffer, ovector_open, matches_open, 1, tagbuf_o, 30);
            if(strcasecmp(tagbuf_o, "page")==0)
            {
                if(mode!=TAG_OUTSIDE) printf("page begin but not outside before\n");

                mode=TAG_PAGE;
            }

            if(strcasecmp(tagbuf_o, "title")==0 && mode==TAG_PAGE)
            {
                if((matches_open=pcre_exec(regex_betweentags, NULL, linebuffer, strlen(linebuffer), 0, PCRE_NOTEMPTY, ovector_open, 30))<1)
                    printf("title empty\n");
                else
                    pcre_copy_substring(linebuffer, ovector_open, matches_open, 2, title, 255);
            }

            if(strcasecmp(tagbuf_o, "revision")==0)
            {
                if(mode!=TAG_PAGE)
                    printf("revision but not in page");

                mode=TAG_REVISION;
            }

            if(strcasecmp(tagbuf_o, "id")==0 && mode==TAG_REVISION)
            {
                if((matches_open=pcre_exec(regex_betweentags, NULL, linebuffer,
                                            strlen(linebuffer), 0, PCRE_NOTEMPTY, ovector_open, 30))<1)
                    printf("id empty on %s\n", title);
                else
                {
                    pcre_copy_substring(linebuffer, ovector_open, matches_open,
                                        2, buffer, 255);
                    id=atoi(buffer);
                }
            }

            if(strcasecmp(tagbuf_o, "contributor")==0)
            {
                if(mode!=TAG_REVISION)
                    printf("contributor but not in revision");

                mode=TAG_CONTRIBUTOR;
            }

            if(strcasecmp(tagbuf_o, "text")==0)
            {
                if(mode!=TAG_REVISION)
                {
                    printf("text but not in revision");
                    if(mode!=TAG_PAGE)
                        printf("and even not in page");
                }

                article[0]=0;                        /* clear article buffer
                                                        */
                article_length=0;                    /* sync length */
                if(matches_open!=3) mode=TAG_TEXT;    /* When there are 3
                                                            matches -> matched
                                                            <text [...]/> */
                else continue;                        /* When 3 matches ->
                                                            no text so go on */

                if((matches_open=pcre_exec(regex_betweentags, NULL, linebuffer,
                                            strlen(linebuffer), 0, 0, ovector_open, 30))<1)
                    printf("text didnt match second time\n");
                else
                {
                    pcre_copy_substring(linebuffer, ovector_open, matches_open,
                                        2, article, 8191);
                    article_length=strlen(article);
                }
            }
        }
        else if(mode==TAG_TEXT && matches_closed<2)
        {
            *(article+article_length)='\n';
            strcpy(article+article_length+1,linebuffer);
            article_length+=(strlen(linebuffer)+1);
            if(article_length>=S_ARTBUF)
            {
                printf("Article buffer overflow! Bad sourcefile or increase buffersize\n");
                return 0;
            }
        }

        if(matches_closed==2)
        {
            pcre_copy_substring(linebuffer, ovector_end, matches_closed, 1,
                                tagbuf_c, 30);
            if(strcasecmp(tagbuf_c, "contributor")==0)
            {
                if(mode!=TAG_CONTRIBUTOR)
                    printf("contributor end but not opened");

                mode=TAG_REVISION;
            }

            if(strcasecmp(tagbuf_c, "revision")==0)
            {
                if(mode!=TAG_REVISION)
                    printf("revision end but not opened");

                mode=TAG_PAGE;
            }

            if(strcasecmp(tagbuf_c, "text")==0)
            {
                if(mode!=TAG_TEXT)
                    printf("text end but not opened");

                mode=TAG_REVISION;
                do_article(id, title, article, article_length);
            }

            if(strcasecmp(tagbuf_c, "page")==0)
            {
                if(mode!=TAG_PAGE)
                    printf("page end but not opened");

                mode=TAG_OUTSIDE;
            }
        }
    }
    fclose(titlefile);
    fclose(redirectfile);
    fclose(outfile);
    fclose(xmldump);

    return 0;
}
