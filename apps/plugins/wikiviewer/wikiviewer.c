/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Frederik M.J. Vestre
 * Copyright (C) 2006 Adam Gashlin
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
#include "plugin.h"
#include <ctype.h>
#include "lib/playback_control.h"
#include <inttypes.h>
#include "mallocer.h" /*mallocer resetable memory allocator*/
#include "mwdb.h"
#include "lib/pluginlib_exit.h"
#if LCD_DEPTH > 1
 #define COLORLINKS
#endif

/* keymappings */

#if (CONFIG_KEYPAD == IPOD_4G_PAD)
#define WIKIVIEWER_MENU BUTTON_MENU
#define WIKIVIEWER_SELECT BUTTON_SELECT
#define WIKIVIEWER_STOP_RECORD (BUTTON_SELECT|BUTTON_REPEAT)
#define WIKIVIEWER_FWD BUTTON_SCROLL_FWD
#define WIKIVIEWER_FWD_REPEAT (BUTTON_SCROLL_FWD|BUTTON_REPEAT)
#define WIKIVIEWER_BACK BUTTON_SCROLL_BACK
#define WIKIVIEWER_BACK_REPEAT (BUTTON_SCROLL_BACK|BUTTON_REPEAT)
#define WIKIVIEWER_PREV BUTTON_LEFT
#define WIKIVIEWER_NEXT BUTTON_RIGHT
#define WIKIVIEWER_RECORD BUTTON_PLAY
#define WIKIVIEWER_RECORD_REPEAT (BUTTON_PLAY|BUTTON_REPEAT)

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define WIKIVIEWER_MENU BUTTON_OFF
#define WIKIVIEWER_SELECT BUTTON_SELECT
#define WIKIVIEWER_STOP_RECORD (BUTTON_REC|BUTTON_MODE)
#define WIKIVIEWER_FWD BUTTON_DOWN
#define WIKIVIEWER_FWD_REPEAT (BUTTON_DOWN|BUTTON_MODE)
#define WIKIVIEWER_BACK BUTTON_UP
#define WIKIVIEWER_BACK_REPEAT (BUTTON_UP|BUTTON_MODE)
#define WIKIVIEWER_PREV BUTTON_LEFT
#define WIKIVIEWER_NEXT BUTTON_RIGHT
#define WIKIVIEWER_RECORD BUTTON_REC
#define WIKIVIEWER_RECORD_REPEAT (BUTTON_REC|BUTTON_MODE)

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define WIKIVIEWER_MENU BUTTON_MENU
#define WIKIVIEWER_SELECT BUTTON_SELECT
#define WIKIVIEWER_STOP_RECORD (BUTTON_SELECT|BUTTON_REPEAT)
#define WIKIVIEWER_FWD BUTTON_DOWN
#define WIKIVIEWER_FWD_REPEAT (BUTTON_DOWN|BUTTON_REPEAT)
#define WIKIVIEWER_BACK BUTTON_UP
#define WIKIVIEWER_BACK_REPEAT (BUTTON_UP|BUTTON_REPEAT)
#define WIKIVIEWER_PREV BUTTON_LEFT
#define WIKIVIEWER_NEXT BUTTON_RIGHT
#define WIKIVIEWER_RECORD BUTTON_A
#define WIKIVIEWER_RECORD_REPEAT (BUTTON_A|BUTTON_REPEAT)

#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#define WIKIVIEWER_MENU BUTTON_MENU
#define WIKIVIEWER_SELECT BUTTON_SELECT
#define WIKIVIEWER_STOP_RECORD (BUTTON_SELECT|BUTTON_REPEAT)
#define WIKIVIEWER_FWD BUTTON_DOWN
#define WIKIVIEWER_FWD_REPEAT (BUTTON_DOWN|BUTTON_REPEAT)
#define WIKIVIEWER_BACK BUTTON_UP
#define WIKIVIEWER_BACK_REPEAT (BUTTON_UP|BUTTON_REPEAT)
#define WIKIVIEWER_PREV BUTTON_LEFT
#define WIKIVIEWER_NEXT BUTTON_RIGHT
#define WIKIVIEWER_RECORD BUTTON_BACK
#define WIKIVIEWER_RECORD_REPEAT (BUTTON_BACK|BUTTON_REPEAT)

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#define WIKIVIEWER_MENU BUTTON_MENU
#define WIKIVIEWER_SELECT BUTTON_SELECT
#define WIKIVIEWER_STOP_RECORD (BUTTON_SELECT|BUTTON_REPEAT)
#define WIKIVIEWER_FWD BUTTON_DOWN
#define WIKIVIEWER_FWD_REPEAT (BUTTON_DOWN|BUTTON_REPEAT)
#define WIKIVIEWER_BACK BUTTON_UP
#define WIKIVIEWER_BACK_REPEAT (BUTTON_UP|BUTTON_REPEAT)
#define WIKIVIEWER_PREV BUTTON_LEFT
#define WIKIVIEWER_NEXT BUTTON_RIGHT
#define WIKIVIEWER_RECORD BUTTON_DISPLAY
#define WIKIVIEWER_RECORD_REPEAT (BUTTON_DISPLAY|BUTTON_REPEAT)

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define WIKIVIEWER_MENU BUTTON_REC
#define WIKIVIEWER_SELECT BUTTON_SELECT
#define WIKIVIEWER_STOP_RECORD (BUTTON_SELECT|BUTTON_REPEAT)
#define WIKIVIEWER_FWD BUTTON_DOWN
#define WIKIVIEWER_FWD_REPEAT (BUTTON_DOWN|BUTTON_REPEAT)
#define WIKIVIEWER_BACK BUTTON_UP
#define WIKIVIEWER_BACK_REPEAT (BUTTON_UP|BUTTON_REPEAT)
#define WIKIVIEWER_PREV BUTTON_LEFT
#define WIKIVIEWER_NEXT BUTTON_RIGHT
#define WIKIVIEWER_RECORD BUTTON_PLAY
#define WIKIVIEWER_RECORD_REPEAT (BUTTON_PLAY|BUTTON_REPEAT)

#elif (CONFIG_KEYPAD == SANSA_C200_PAD)
#define WIKIVIEWER_MENU BUTTON_POWER
#define WIKIVIEWER_SELECT BUTTON_SELECT
#define WIKIVIEWER_STOP_RECORD (BUTTON_SELECT|BUTTON_REPEAT)
#define WIKIVIEWER_FWD BUTTON_DOWN
#define WIKIVIEWER_FWD_REPEAT (BUTTON_DOWN|BUTTON_REPEAT)
#define WIKIVIEWER_FWD2 BUTTON_VOL_UP
#define WIKIVIEWER_FWD2_REPEAT (BUTTON_VOL_UP|BUTTON_REPEAT)
#define WIKIVIEWER_BACK BUTTON_UP
#define WIKIVIEWER_BACK_REPEAT (BUTTON_UP|BUTTON_REPEAT)
#define WIKIVIEWER_BACK2 BUTTON_VOL_DOWN
#define WIKIVIEWER_BACK2_REPEAT (BUTTON_VOL_DOWN|BUTTON_REPEAT)
#define WIKIVIEWER_PREV BUTTON_LEFT
#define WIKIVIEWER_NEXT BUTTON_RIGHT
#define WIKIVIEWER_RECORD BUTTON_REC
#define WIKIVIEWER_RECORD_REPEAT (BUTTON_REC|BUTTON_REPEAT)

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#define WIKIVIEWER_MENU BUTTON_POWER
#define WIKIVIEWER_SELECT BUTTON_SELECT
#define WIKIVIEWER_STOP_RECORD (BUTTON_SELECT|BUTTON_REPEAT)
#define WIKIVIEWER_FWD BUTTON_DOWN
#define WIKIVIEWER_FWD_REPEAT (BUTTON_DOWN|BUTTON_REPEAT)
#define WIKIVIEWER_FWD2 BUTTON_SCROLL_FWD
#define WIKIVIEWER_FWD2_REPEAT (BUTTON_SCROLL_FWD|BUTTON_REPEAT)
#define WIKIVIEWER_BACK BUTTON_UP
#define WIKIVIEWER_BACK_REPEAT (BUTTON_UP|BUTTON_REPEAT)
#define WIKIVIEWER_BACK2 BUTTON_SCROLL_BACK
#define WIKIVIEWER_BACK2_REPEAT (BUTTON_SCROLL_BACK|BUTTON_REPEAT)
#define WIKIVIEWER_PREV BUTTON_LEFT
#define WIKIVIEWER_NEXT BUTTON_RIGHT
#define WIKIVIEWER_RECORD BUTTON_REC
#define WIKIVIEWER_RECORD_REPEAT (BUTTON_REC|BUTTON_REPEAT)

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#define WIKIVIEWER_MENU BUTTON_HOME
#define WIKIVIEWER_SELECT BUTTON_SELECT
/* #define WIKIVIEWER_STOP_RECORD (BUTTON_SELECT|BUTTON_REPEAT) */
#define WIKIVIEWER_FWD BUTTON_DOWN
#define WIKIVIEWER_FWD_REPEAT (BUTTON_DOWN|BUTTON_REPEAT)
#define WIKIVIEWER_FWD2 BUTTON_SCROLL_FWD
#define WIKIVIEWER_FWD2_REPEAT (BUTTON_SCROLL_FWD|BUTTON_REPEAT)
#define WIKIVIEWER_BACK BUTTON_UP
#define WIKIVIEWER_BACK_REPEAT (BUTTON_UP|BUTTON_REPEAT)
#define WIKIVIEWER_BACK2 BUTTON_SCROLL_BACK
#define WIKIVIEWER_BACK2_REPEAT (BUTTON_SCROLL_BACK|BUTTON_REPEAT)
#define WIKIVIEWER_PREV BUTTON_LEFT
#define WIKIVIEWER_NEXT BUTTON_RIGHT
/* #define WIKIVIEWER_RECORD (BUTTON_SELECT|BUTTON_REPEAT) */
/* #define WIKIVIEWER_RECORD_REPEAT (BUTTON_HOME|BUTTON_REPEAT) */

#elif (CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD)
#define WIKIVIEWER_MENU BUTTON_POWER
#define WIKIVIEWER_SELECT BUTTON_SELECT
/* #define WIKIVIEWER_STOP_RECORD (BUTTON_SELECT|BUTTON_REPEAT) */
#define WIKIVIEWER_FWD BUTTON_DOWN
#define WIKIVIEWER_FWD_REPEAT (BUTTON_DOWN|BUTTON_REPEAT)
#define WIKIVIEWER_FWD2 BUTTON_PLAYPAUSE
#define WIKIVIEWER_FWD2_REPEAT (BUTTON_PLAYPAUSE|BUTTON_REPEAT)
#define WIKIVIEWER_BACK BUTTON_UP
#define WIKIVIEWER_BACK_REPEAT (BUTTON_UP|BUTTON_REPEAT)
#define WIKIVIEWER_BACK2 BUTTON_BACK
#define WIKIVIEWER_BACK2_REPEAT (BUTTON_BACK|BUTTON_REPEAT)
#define WIKIVIEWER_PREV BUTTON_LEFT
#define WIKIVIEWER_NEXT BUTTON_RIGHT
/* #define WIKIVIEWER_RECORD (BUTTON_SELECT|BUTTON_REPEAT) */
/* #define WIKIVIEWER_RECORD_REPEAT (BUTTON_HOME|BUTTON_REPEAT) */

#elif (CONFIG_KEYPAD == ONDAVX747_PAD)
#define WIKIVIEWER_MENU BUTTON_POWER

#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef WIKIVIEWER_SELECT
#define WIKIVIEWER_SELECT BUTTON_CENTER
#endif
#ifndef WIKIVIEWER_MENU
#define WIKIVIEWER_MENU BUTTON_TOPLEFT
#endif
#ifndef WIKIVIEWER_FWD
#define WIKIVIEWER_FWD BUTTON_BOTTOMMIDDLE
#define WIKIVIEWER_FWD_REPEAT (BUTTON_BOTTOMMIDDLE|BUTTON_REPEAT)
#endif
#ifndef WIKIVIEWER_BACK
#define WIKIVIEWER_BACK BUTTON_TOPMIDDLE
#define WIKIVIEWER_BACK_REPEAT (BUTTON_TOPMIDDLE|BUTTON_REPEAT)
#endif
#ifndef WIKIVIEWER_PREV
#define WIKIVIEWER_PREV BUTTON_MIDLEFT
#endif
#ifndef WIKIVIEWER_NEXT
#define WIKIVIEWER_NEXT BUTTON_MIDRIGHT
#endif
#endif

#define LINEBUFLEN 255
#define MARKUP_STARTULINE   1
#define MARKUP_ENDULINE     2
#define MARKUP_STARTBOLD    3
#define MARKUP_ENDBOLD      4
#define MARKUP_STARTITALIC  5
#define MARKUP_ENDITALIC    6
#define MARKUP_STARTLINK    7
#define MARKUP_ENDLINK      8
#define MARKUP_LINEFEED     10
#define MARKUP_MODE     15


/* change on any change to file format, histstate */
#define SAVE_FILE_VERSION_MAGIC 0x12340004

static bool wrquit=false;
static char filename[MAX_PATH+1];      /* name of database, without extension */

#define MAINMEMBUF 0


static void *inflatebuf;           /* heap for gunzip */
static char *articlebuf;           /* destination for uncompressed article */
static uint32_t articlebuflen=0;   /* size of the article buffer */
static uint32_t articlelen=0;      /* used size of the article buffer */

static int lastlinkcount=0;        /* set by render() to the number of links
                                      found */

/* provides everything render() needs to know to render a screen */
struct linestate {
    int32_t renderoff;
    int renderlen;
    int32_t linknameoff;
    int inlink,inlinkname;
#ifdef COLORLINKS
    uint8_t inem;
#endif
    int eof; /* should be interpreted as disallowing further scrolling down */
};

#define ARTICLENAMEBUF_LENGTH 255

/* where we'll be scrolling next */
static struct linestate curlinestate,nextlinestate;

/* scrollback, to allow scrolling backwards fairly quickly (without some memory
   entire article might need to be reparsed to find context of previous line)
   stack, up to SCROLLBACK_LENGTH, past that the oldest entry is discarded */
#define SCROLLBACK_LENGTH 50

static int curlinehist,linehistdepth;
static int curline;
static struct linestate linehist[SCROLLBACK_LENGTH];

/* bookmarks, for going back to a previous article */

struct bookstate {
    char name[ARTICLENAMEBUF_LENGTH];
    int32_t curoff;
    uint32_t res_lo;   /* avoid searching again */
    uint32_t res_hi;
};


/* history, for going back to a previous article */

struct histstate {
    char name[ARTICLENAMEBUF_LENGTH];
    int32_t curoff;
    uint32_t res_lo;   /* avoid searching again */
    uint32_t res_hi;
};

/*
   It should be possible to reconstruct a linestate from a histstate as follows:
   1) load the article given by res_lo, res_hi 2) linestate=0 3) while
   linestate.renderoff<histstate.curoff && !eof, advance
 */

/* stack, up to HISTORY_LENGTH deep, past that oldest entry is discarded */
/* the current article occupies hist[curhist] */
#define HISTORY_LENGTH 20

static int curhist,histdepth;
static int bookdepth;
static bool record_stop_flag=true;
static struct histstate hist[HISTORY_LENGTH];
static struct bookstate book[200];
static int record_flag=0;   /*set to 1 when recording a line to an external
                               file*/
static int last_line_i=0;   /*set to linespace when recording a line to an
                               external file*/
static char last_line[256]; /* the current line of text to be recorded */
static int end_of_file=0;   /*flag for end of file in recording*/
static int record_linebreak=0; /*flag for line break in recorded line*/

static void viewer_exit(void *parameter);
static struct linestate render(struct linestate cur, int norender,
                               int linktonote);
static void readlink(struct linestate cur, int link, char * namebuf,
                     int buflen);

static void viewer_exit(void *parameter)
{
    (void)parameter;
}

static void set_article_offset(int32_t off);
static void reset_scrollback(void);
static void advance_scrollback(int a, int dorender);
static void reset_history(void);
static void advance_history(int d);

static int load_bookmarks(const char * filename)
{
    char fnbuf[MAX_PATH+1];
    int fd, i, booki, ret = 0;
    uint32_t magic=0;

    rb->snprintf(fnbuf,MAX_PATH,"%s.wwb",filename);
    fd = rb->open(fnbuf, O_RDONLY);

    if (fd<0)
        goto err;

    if (rb->read(fd,&magic,sizeof(magic))<=0)
        goto err;

    if (magic!=SAVE_FILE_VERSION_MAGIC)
        goto err;

    if (rb->read(fd,&booki,sizeof(booki))<=0)
        goto err;

    if (booki<=0)
        goto err;

    for (i=0; i<booki; i++)
    {
        if (rb->read(fd,book+i,sizeof(struct bookstate))<=0)
            goto err;
    }

    bookdepth = booki;

    ret = 1;

err:
    if(fd >= 0)
        rb->close(fd);

    return ret;
}

static void save_bookmark(const char * filename)
{
    char fnbuf[MAX_PATH+1];
    int fd;
    int i,booki;
    uint32_t magic;

    rb->snprintf(fnbuf,MAX_PATH,"%s.wwb",filename);

    fd = rb->open(fnbuf, O_WRONLY|O_CREAT);
    if (fd<0) return;

    magic=SAVE_FILE_VERSION_MAGIC;
    booki=0;

    rb->write(fd, &magic, sizeof(magic));
    rb->write(fd, &bookdepth, sizeof(bookdepth));

    for (i=0; i<bookdepth; i++)
    {
        rb->write(fd, book+booki, sizeof(struct bookstate));
        booki=(booki+1);
    }

    rb->close(fd);
}

static void add_bookmark(void)
{
    rb->strcpy(book[bookdepth].name,hist[curhist].name);
    book[bookdepth].curoff=hist[curhist].curoff;
    book[bookdepth].res_lo=hist[curhist].res_lo;
    book[bookdepth].res_hi=hist[curhist].res_hi;
    bookdepth++;
}

static void render_bookmarks(int off)
{
    int i=(bookdepth-1),j,d;
    int x,y,fontheight,t;
    char buf[10];

    rb->lcd_clear_display();

    y=1;
    fontheight = rb->font_get(FONT_UI)->height;

    for (j=1,d=(bookdepth); d>0 && y+fontheight<LCD_HEIGHT; d--,j++)
    {
        if (j>=(off+1))
        {
            rb->snprintf(buf,10,"%3d. ",j);
            rb->lcd_putsxy(1,y,buf);
            rb->lcd_getstringsize(buf,&x,&t);
            rb->lcd_putsxy(1+x,y,book[i].name);
            y+=fontheight;
        }

        i--;
    }

    rb->lcd_update();
}

static void remove_bookmark(int bookviewoff)
{
    int i,booki;

    booki=bookdepth-bookviewoff-1;
    for (i=booki; i<bookdepth; i++)
    {
        rb->strcpy(book[i].name,book[(i+1)].name);
        book[i].curoff=book[(i+1)].curoff;
        book[i].res_lo=book[(i+1)].res_lo;
        book[i].res_hi=book[(i+1)].res_hi;
    }
    bookdepth--;
}

static void save_status(const char * filename)
{
    char fnbuf[MAX_PATH+1];
    int fd;
    int i,histi;
    uint32_t magic;

    rb->snprintf(fnbuf,MAX_PATH,"%s.wws",filename);

    fd = rb->open(fnbuf, O_WRONLY|O_CREAT);
    if (fd<0) return;

    magic=SAVE_FILE_VERSION_MAGIC;

    rb->write(fd, &magic, sizeof(magic));
    rb->write(fd, &histdepth, sizeof(histdepth));

    histi=curhist-(histdepth-1);
    if (histi<0) histi+=HISTORY_LENGTH;

    for (i=0; i<HISTORY_LENGTH; i++)
    {
        rb->write(fd, hist+histi, sizeof(struct histstate));
        histi=(histi+1)%HISTORY_LENGTH;
    }

    rb->close(fd);
}

static int load_status(const char * filename)
{
    char fnbuf[MAX_PATH+1];
    int fd, i, histi, ret = 0;
    uint32_t magic=0;

    rb->snprintf(fnbuf,MAX_PATH,"%s.wws",filename);
    fd = rb->open(fnbuf, O_RDONLY);
    if (fd<0)
        goto err;

    if (rb->read(fd,&magic,sizeof(magic))<=0)
        goto err;

    if (magic!=SAVE_FILE_VERSION_MAGIC)
        goto err;

    if (rb->read(fd,&histi,sizeof(histi))<=0)
        goto err;

    if (histi<=0)
        goto err;

    for (i=0; i<histi; i++)
    {
        if (rb->read(fd,hist+i,sizeof(struct histstate))<=0)
            goto err;
    }

    histdepth=histi;
    curhist=i-1;

    ret = 1;

err:
    if(fd >= 0)
        rb->close(fd);

    return ret;
}

static void set_article_offset(int32_t off)
{
    /* go to offset off in current article */

    reset_scrollback();
    curline=0;
    rb->memset(&curlinestate,0,sizeof(curlinestate));
    rb->memset(&nextlinestate,0,sizeof(nextlinestate));

    hist[curhist].curoff=curlinestate.renderoff=10+articlebuf[8];
    curlinestate.renderlen=articlelen-10-articlebuf[8];

    nextlinestate=render(curlinestate,1,0);

    while (!nextlinestate.eof && hist[curhist].curoff<off)
    {
        advance_scrollback(1,0);
    }
}

static void reset_scrollback(void)
{
    curlinehist=0;
    linehistdepth=0;
}

/* all scrolling done through this */
static void advance_scrollback(int a, int dorender)
{
    int i;
    if (a<-1)
    {
        if ((a+curline)<0)
            a=(-curline);

        for (i=0; i<-a; i++)
        {
            advance_scrollback(-1,(dorender && i==-a-1));
        }
    }
    else if (a>1)
        for (i=0; i<a; i++)
        {
            advance_scrollback(1,(dorender && i==a-1));
        }
    else if (a==0)
        return;

    else if (a<0)       /* a==-1 */
    {   /* scroll backwards one line */
        if (curline==0) return;

        curline--;

        if (linehistdepth>0)
        {
            /* if we have records to fall back on */
            linehistdepth--;
            curlinehist--;

            if (curlinehist<0)
                curlinehist=SCROLLBACK_LENGTH-1;

            curlinestate=linehist[curlinehist];
            nextlinestate=render(curlinestate,!dorender,0);
            hist[curhist].curoff=curlinestate.renderoff;
        }
        else
        {
            int amounttoscroll=curline;
            /* redo from start */
            reset_scrollback(); /* clear scrollback */
            set_article_offset(0);    /* go to top */
            advance_scrollback(amounttoscroll,dorender); /* scoll down */
        }
    }
    else            /* a==1 */
    /* scroll forwards one line */
    if (!nextlinestate.eof || ((record_flag==1) && (end_of_file==0)))
    {
        linehist[curlinehist]=curlinestate;
        curlinehist=(curlinehist+1)%SCROLLBACK_LENGTH;
        linehistdepth++;

        if (linehistdepth>SCROLLBACK_LENGTH)
            linehistdepth=SCROLLBACK_LENGTH;

        curlinestate=nextlinestate;
        curline++;
        nextlinestate=render(curlinestate,!dorender,0);
        hist[curhist].curoff=curlinestate.renderoff;
    }
}

static void reset_history(void)
{
    curhist=0;
    histdepth=1;
}

static void advance_history(int d)
{
    if (d==1)
    {
        curhist=(curhist+1)%HISTORY_LENGTH;
        histdepth++;
        if (histdepth>HISTORY_LENGTH) histdepth=HISTORY_LENGTH;
    }
    else if (d==-1)
        if (histdepth>1)
        {
            curhist--;
            if (curhist<0) curhist=HISTORY_LENGTH-1;

            histdepth--;
        }
}

static int iswhitespace(uint16_t c)
{
    return (c==' ' || c=='\n');     /* what about tab character? */
}

static int islinebreak(uint16_t c)
{
    return (c=='\n');
}

/* return next place to scroll to */
static struct linestate render(struct linestate cur, int norender,
                               int linktonote)
{
    int i;                        /* offset in buf */
    int x,y;                      /* current render location */
    const char * nextchar;        /* character after the current UTF-8 character
                                   */
    int charsize;                 /* size of the current UTF-8 character */
    uint16_t ucs;                 /* current UTF-8 character */
    char buf[LINEBUFLEN+1];       /* the current line of text to be rendered */
    uint8_t underline[LCD_WIDTH]; /* should there be an underline at this pixel?
                                   */

    int fontheight;
    int first = 0;           /*record only first line through*/
    char fnbuf1[MAX_PATH+1]; /*for recording line of text*/
    int fd1;                 /*for recording line of text*/

    end_of_file=0;

#ifdef COLORLINKS
    uint8_t em[LCD_WIDTH];  /* type of emphasis for this pixel */
    unsigned lastcol=0;
    rb->memset(em,0,sizeof(em));
#endif

    struct linestate nextline;  /* state to return */

    /* state to back up to when doing line break */
    int lastspace_i=0;
    int lastspace_linkcount=0;
    struct linestate lastspace;

    int linkcount;
    int linedone=0;
    /* if the last character was whitespace we'll ignore subsequent whitespace
     */
    int lastwaswhitespace;

    fontheight = rb->font_get(FONT_UI)->height;
    y=1;

    linkcount=1;
    lastlinkcount=0;

    nextline.renderoff=-1;
    /* it's not going to be any less the end... */
    nextline.eof=cur.eof;

    if (!norender) rb->memset(underline,0,sizeof(underline));

    while (((y+fontheight) < LCD_HEIGHT) && cur.renderlen>0)
    {
        lastspace.renderoff=-1;
        ucs=0;

        /* prevent whitespace from appearing at the beginning of a line */
        lastwaswhitespace=1;

        /* scroll halfway down the screen */
        /* if (y==(LCD_HEIGHT/fontheight/2)*fontheight+1) halfway=article; */
        /* see how much we can fit on a line */

        for (x=1,i=0,linedone=0; !linedone && i<LINEBUFLEN && cur.renderlen>0; )
        {
            /*handle markup*/
            switch (articlebuf[cur.renderoff])
            {
            case MARKUP_STARTULINE:
            case MARKUP_STARTBOLD:
            case MARKUP_STARTITALIC:

#ifdef COLORLINKS
                switch (articlebuf[cur.renderoff])
                {
                case MARKUP_STARTULINE:
                    cur.inem=1;
                    break;
                case MARKUP_STARTBOLD:
                    cur.inem=2;
                    break;
                case MARKUP_STARTITALIC:
                    cur.inem=3;
                    break;
                }
#endif
                cur.renderlen--;
                cur.renderoff++;
                break;

            case MARKUP_ENDULINE:
            case MARKUP_ENDBOLD:
            case MARKUP_ENDITALIC:

#ifdef COLORLINKS
                if (cur.inem)
                    cur.inem=0;

#endif

                cur.renderlen--;
                cur.renderoff++;
                break;

            case MARKUP_STARTLINK:
                cur.inlink=1;
                cur.linknameoff=cur.renderoff;
                cur.renderlen--;
                cur.renderoff++;
                break;

            case MARKUP_ENDLINK:
                if (cur.inlinkname)
                {
                    cur.inlinkname=0;
                    linkcount++;
                }

                if (cur.inlink)
                {
                    /* start outputting link text */
                    cur.renderlen+=cur.renderoff-cur.linknameoff;
                    cur.renderoff-=cur.renderoff-cur.linknameoff;
                    cur.linknameoff=0;
                    cur.inlink=0;
                    cur.inlinkname=1;
                }

                cur.renderlen--;
                cur.renderoff++;
                break;

            case MARKUP_MODE:
                if (cur.inlink) cur.linknameoff=cur.renderoff;

                cur.renderlen--;
                cur.renderoff++;
                break;

            default:
                nextchar = rb->utf8decode(articlebuf+cur.renderoff,&ucs);
                charsize = nextchar-cur.renderoff-articlebuf;
                /* multiple newlines work */
                if (islinebreak(ucs)) linedone=1;  /* break;*/  /* but multiple
                                                      other whitespace will be
                                                      ignored */

                if (!linedone && !cur.inlink && !(iswhitespace(ucs) &&
                                                  lastwaswhitespace))
                {
                    /* display */
                    int charwidth=rb->font_get_width(rb->font_get(FONT_UI),ucs);

                    if (iswhitespace(ucs))
                    {
                        lastspace=cur;
                        lastspace_i=i;
                        lastspace_linkcount=linkcount;
                        lastwaswhitespace=1;
                    }
                    else lastwaswhitespace=0;

                    if ((x+=charwidth) > LCD_WIDTH)
                    {
                        linedone=1;
                        break;
                    }

                    if (!norender)
                    {
                        rb->memcpy(buf+i,articlebuf+cur.renderoff,charsize);
                        if (cur.inlinkname)
                            rb->memset(underline+(x-charwidth),linkcount,
                                       charwidth);

#ifdef COLORLINKS
                        if (cur.inem)
                            rb->memset(em+(x-charwidth),cur.inem,charwidth);

#endif
                    }

                    i+=charsize;
                }
                else
                {
                    /* hidden */
                }

                cur.renderlen-=charsize;
                cur.renderoff+=charsize;
            } /* end markup switch */
        } /* end for characters in a line */

        if (x>=LCD_WIDTH)
        {
            /* the next character would be offscreen, terminate at previous
               space */
            if (lastspace.renderoff==-1)
            {
                /* if we have a long word, break it here */
            }
            else
            {
                cur=lastspace;
                linkcount=lastspace_linkcount;
                i=lastspace_i;
            }
        }

        if (!norender)
        {
            int uc,ucm,t;

            if (first<1)
            {
                if (record_flag==1)
                {
                    record_flag=0;

                    rb->snprintf(fnbuf1,MAX_PATH,"/wiki/%s.txt",hist[curhist].name);
                    fd1 = rb->open(fnbuf1, O_WRONLY|O_APPEND|O_CREAT);

                    if (fd1<0)
                        rb->splash(HZ*2, "Recording NOT done");
                    else
                    {
                        if (record_stop_flag)
                        {
                            rb->write(fd1,"\n\r",2);
                            record_stop_flag=false;
                        }

                        rb->write(fd1,(unsigned char *)last_line,last_line_i);
                        if (record_linebreak>0)
                            rb->write(fd1,"\n",2);
                        else
                            rb->write(fd1," ",1);

                        rb->close(fd1);
                    }
                }

                last_line_i=i;
                rb->memcpy(last_line,buf,(last_line_i));
                if (islinebreak(ucs))
                    record_linebreak=1;
                else
                    record_linebreak=0;
            }

            first++;

            buf[(i<=LINEBUFLEN) ? i : LINEBUFLEN]='\0';
            rb->lcd_putsxy(1,y, (unsigned char *)buf);
            rb->lcd_getstringsize(buf,&ucm,&t);

            int oldmode=rb->lcd_get_drawmode();

            /* mark links */
            for (uc=0; uc<LCD_WIDTH; uc++)
            {
#ifdef COLORLINKS
                /* mark emphasis */
                if (em[uc])
                {
                    lastcol=rb->lcd_get_foreground();
                    rb->lcd_set_drawmode(DRMODE_SOLID);
                    switch (em[uc])
                    {
                    case 1:
                        rb->lcd_set_foreground(LCD_RGBPACK(255,0,0));
                        break;
                    case 2:
                        rb->lcd_set_foreground(LCD_RGBPACK(0,255,0));
                        break;
                    case 3:
                        rb->lcd_set_foreground(LCD_RGBPACK(0,0,255));
                        break;
                    }
                    rb->lcd_drawpixel(uc,y+fontheight-1);
                    rb->lcd_set_foreground(lastcol);
                }

                em[uc]=0;
#endif

                if (underline[uc] && uc > 1 && uc < (ucm+1))
                {
                    if (underline[uc]==linktonote)
                    {
                        rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
                        rb->lcd_vline(uc,y,y+fontheight-1);
                    }
                    else
                    {
                        rb->lcd_set_drawmode(DRMODE_SOLID);
                        rb->lcd_drawpixel(uc,y+fontheight-1);
                    }

                    lastlinkcount=underline[uc];
                }

                underline[uc]=0;
                /* clear rest of line */
                if (uc >= ucm+1)
                {
                    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                    rb->lcd_vline(uc,y,y+fontheight-1);
                }
            }

            rb->lcd_set_drawmode(oldmode);
        }

        y+=fontheight;
        /* scroll one line */
        if (nextline.renderoff==-1)
        {
            nextline=cur;
            if (norender) return nextline;
        }
    }
    if (cur.renderlen<=0)
    {
        if (first<2)
            end_of_file=1;

        cur.eof=1;
        nextline.eof=1;
        while ((y+fontheight) < LCD_HEIGHT)
        {
            rb->lcd_putsxy(1,y,"                                              ");
            y+=fontheight;
        }
    }

    if (!norender)
        rb->lcd_update();

    return ((nextline.renderoff==-1) ? cur : nextline);
}

static void readlink(struct linestate cur, int link, char * namebuf, int buflen)
{
    int linkcount=0;

    rb->memset(namebuf,0,buflen);

    if (!link)
        return;

    /*
       screen starts partway through a link
     */
    if (cur.inlink || cur.inlinkname)
        /* seek back to the start of the link */
        while (articlebuf[cur.renderoff] != MARKUP_STARTLINK)
        {
            cur.renderoff--;
            cur.renderlen++;
        }

    while (cur.renderlen)
    {
        if (articlebuf[cur.renderoff] == MARKUP_STARTLINK)
        {
            linkcount++;
            if (linkcount==link)
            {
                cur.renderoff++;
                cur.renderlen--;
                break;
            }
        }

        cur.renderoff++;
        cur.renderlen--;
    }
    if (linkcount==link)
    {
        int i;
        for (i=0; i<buflen-1 && cur.renderlen>0; )
        {
            int charsize;
            if (articlebuf[cur.renderoff]==MARKUP_ENDLINK || articlebuf[cur.renderoff]==MARKUP_MODE) break;

            charsize = rb->utf8seek(articlebuf+cur.renderoff,1);
            rb->memcpy(namebuf+i,articlebuf+cur.renderoff,charsize);
            cur.renderlen-=charsize;
            i+=charsize;
            cur.renderoff+=charsize;
        }
    }
}

/* locate headings, indicated by a line beginning with = */
/* this is unfortunately quite stupid about markup in headings */

static int render_outline(int inoff, int norender)
{
    int renderoff;
    int renderlen;
    int nextlineoff;
    int linecount;
    int intdepth=0;
    int extdepth=0;
    int linestart;
    int charsize;
    int fontheight;
    int y;

    char preserv;
    enum {
        STATE_START, /* start of line */
        STATE_DD,    /* determining depth */
        STATE_DD2,   /* check closing =s */
        STATE_HTEXT, /* heading text */
        STATE_OTHERTEXT /* other text */
    } state;

    if (inoff<10) inoff=10;

    renderoff=inoff;
    renderlen=articlelen-renderoff;
    linecount=0;

    y=1;
    fontheight=rb->font_get(FONT_UI)->height;

    if (!norender)
        rb->lcd_clear_display();

    if (renderoff==10)   /* we will include the article title */
    {
        if (!norender)
        {
            preserv=articlebuf[articlebuf[8]+10];
            articlebuf[articlebuf[8]+10]='\0';
            rb->lcd_putsxy(1,y,articlebuf+10);

            articlebuf[articlebuf[8]+10]=preserv;
        }

        y+=fontheight;
        linecount++;

        renderlen-=articlebuf[8];
        renderoff+=articlebuf[8];
    }

    nextlineoff=renderoff;

    linestart=renderoff;
    state=STATE_START;

    while (renderlen>0 && y+fontheight < LCD_HEIGHT)
    {
        switch (state)
        {
        case STATE_START:
            linestart=renderoff;
            switch (articlebuf[renderoff])
            {
            case '=':
                if (linecount==1)
                {
                    nextlineoff=renderoff;         /* beginning of second line
                                                    */
                    if (norender) return nextlineoff;
                }

                intdepth=1;
                state=STATE_DD;
                break;
            case MARKUP_LINEFEED:
                break;
            default:
                state=STATE_OTHERTEXT;
                break;
            }
            break;

        case STATE_DD:
            switch (articlebuf[renderoff])
            {
            case '=':
                intdepth++;
                break;
            case MARKUP_LINEFEED:
                state=STATE_START;         /* give up */
                break;
            default:
                state=STATE_HTEXT;
                break;
            }
            break;

        case STATE_DD2:
            switch (articlebuf[renderoff])
            {
            case '=':
                extdepth++;
                break;
            case MARKUP_LINEFEED:
                /* final acceptance here */
                if (!norender)
                {
                    if (linecount==0)
                        rb->lcd_clear_display();

                    int depth;
                    if(extdepth<intdepth)
                        depth=extdepth;
                    else
                        depth=intdepth;

                    int whitey=0;
                    preserv=articlebuf[renderoff-depth];
                    articlebuf[renderoff-depth]='\0';

                    /* trim leading whitespace */

                    while (iswhitespace(articlebuf[linestart+depth+
                                                   whitey]))
                    {
                        charsize = rb->utf8seek(articlebuf+linestart+
                                                depth+whitey,1);

                        whitey+=charsize;
                    }

                    rb->lcd_putsxy(1+depth*rb->font_get_width(
                                       rb->font_get(FONT_UI),'_'),y,
                                   articlebuf+linestart+depth+whitey);

                    articlebuf[renderoff-depth]=preserv;
                }

                linecount++;
                y+=fontheight;
                state=STATE_START;
                break;

            default:
                /* return to HTEXT, might be an = within a heading */
                state=STATE_HTEXT;
                break;
            }
            break;

        case STATE_HTEXT:
            switch (articlebuf[renderoff])
            {
            case '=':
                extdepth=1;
                state=STATE_DD2;
                break;

            case MARKUP_LINEFEED:
                state=STATE_START;         /* give up */
                break;

            default:
                /* continue to accept text within the heading */
                break;
            }
            break;

        case STATE_OTHERTEXT:
            switch (articlebuf[renderoff])
            {
            case MARKUP_LINEFEED:
                state=STATE_START;
                break;

            default:
                break;
            }
            break;
        } /* end switch (state) */

        charsize = rb->utf8seek(articlebuf+renderoff,1);
        renderlen-=charsize;
        renderoff+=charsize;
    }

    if (!norender)
        rb->lcd_update();

    return nextlineoff;
}

static void render_history(int off)
{
    int i=curhist,j,d;
    int x,y,fontheight,t;
    char buf[10];

    rb->lcd_clear_display();

    y=1;
    fontheight = rb->font_get(FONT_UI)->height;

    for (j=1,d=histdepth; d>0 && y+fontheight<LCD_HEIGHT; d--,j++)
    {
        if (j>=(off+1))
        {
            rb->snprintf(buf,10,"%2d. ",j);
            rb->lcd_putsxy(1,y,buf);
            rb->lcd_getstringsize(buf,&x,&t);
            rb->lcd_putsxy(1+x,y,hist[i].name);
            y+=fontheight;
        }

        i--;
        if (i<0)
            i=HISTORY_LENGTH-1;
    }

    rb->lcd_update();
}

/* read 2 character ascii hex */
static int readhex(unsigned char * buf)
{
    int t=0;
    if (buf[0]>='0'&&buf[0]<='9')
        t=(buf[0]-'0')*16;
    else if (buf[0]>='A'&&buf[0]<='F')
        t=(buf[0]-'A'+10)*16;
    else return -1;

    if (buf[1]>='0'&&buf[1]<='9')
        t+=(buf[1]-'0');
    else if (buf[1]>='A'&&buf[1]<='F')
        t+=(buf[1]-'A'+10);
    else return -1;

    return t;
}

static bool viewer_init(void)
{
    enum {
        MODE_LINK,
        MODE_NAVVIEW,
        MODE_HISTVIEW,
        MODE_NORMAL,
        MODE_BOOKVIEW
    } mode;

    int linkno;      /* currently selected link */
    int navcur;      /* current offset for navigation */
    int navnext;     /* next offset for navigation mode */
    int navline;     /* current line for navigation (important for scrolling up)
                      */
    int histviewoff; /* how far back */
    int bookviewoff; /* how far back */
    int fontheight;
    fontheight = rb->font_get(FONT_UI)->height;

    char * target;   /* an anchor to seek out */

    bool prompt=1;   /* prompt for article name? */
    bool dofind=1;   /* run mwdb_findarticle? */
    curline=0;       /* current line */

    reset_history();
    hist[curhist].curoff=0; /* first article starts at beginning */

    /* allocate memory */
    wpw_init_mempool(MAINMEMBUF);
    inflatebuf=wpw_malloc(MAINMEMBUF,0x13500);
    articlebuflen=wpw_available(MAINMEMBUF)>0x32000 ? 0x32000 : wpw_available(MAINMEMBUF);
    articlebuf=wpw_malloc(MAINMEMBUF,articlebuflen);

    /* initially no name in prompt, subsequently default to previous name */
    rb->memset(hist[curhist].name,0,ARTICLENAMEBUF_LENGTH);

#ifdef HAVE_TOUCHSCREEN
    rb->touchscreen_set_mode(TOUCHSCREEN_POINT);
#endif

    if (load_status(filename))
    {
        prompt=0;
        dofind=0;
    }

    if (load_bookmarks(filename))                    /*UNSURE*/
    {
    }

    target=0;

loadnewarticle:
    if (prompt || dofind)
        if (!mwdb_findarticle(filename,hist[curhist].name,ARTICLENAMEBUF_LENGTH,
                              &hist[curhist].res_lo,&hist[curhist].res_hi,true,
                              prompt,1))
        {
            /* try case-insensitive match if case-sensitive match fails */
            if (!rb->strlen(hist[curhist].name))
            {
                /* bailed out */

                /* automatically go back */
                if (histdepth>1)
                {
                    prompt=0;
                    dofind=0;
                    advance_history(-1);

                    goto loadnewarticle;    /* few lines up */
                }

                return true;
            }

            if (!mwdb_findarticle(filename,hist[curhist].name,
                                  ARTICLENAMEBUF_LENGTH,&hist[curhist].res_lo,
                                  &hist[curhist].res_hi,true,0,0))
            {
                rb->splashf(HZ*2, "didn't find \"%s\"",hist[curhist].name);

                /* automatically go back */
                if (histdepth>1)
                {
                    if (prompt==0)
                    {
                        prompt=0;
                        dofind=0;
                        advance_history(-1);
                    }

                    goto loadnewarticle;        /* few lines up */
                }

                return true; /* No reason to make the plugin fail */
            }
        }


    prompt=0;

    if (!mwdb_loadarticle(filename,inflatebuf,articlebuf,articlebuflen,
                          &articlelen,hist[curhist].res_lo,hist[curhist].res_hi))
        return false;  /* load error is a problem */

    rb->lcd_clear_display();

    /* if a target has been specified */
    if (target)
    {
        int i,j;

        /* find headings */
        navcur=-1;
        navnext=0;
        while (navcur!=navnext)
        {
            navcur=navnext;
            navnext=render_outline(navcur,1);
            /* navnext will be pointing at the start of the line with the target
               in it

             * Attempt match, this is a bit tricky (and it is probably equally
             *tricky to get an anchor pointing to another document in wikicode
             *in the first place).
             */
            for (i=0,j=navnext; target[i] && articlebuf[j]; )
            {
                if (target[i]==articlebuf[j])
                {
                    i++; j++;
                }
                else
                {
                    if (i==0 && (articlebuf[j]=='=' || articlebuf[j]==' '))
                        j++;  /* ignore starting =s, spaces */
                    else if (target[i]=='_' && articlebuf[j]==' ')
                    {
                        i++;
                        j++; /* spaces converted to underscores */
                    }
                    else if (target[i]=='.')   /* check for escape sequences */
                    {
                        if (articlebuf[j]==readhex(target+i+1))
                        {
                            i+=3;
                            j++;
                        }
                        else break;
                    }
                    else break;
                }
            }
            if (target[i])
                continue;

            while (articlebuf[j]==' ')
                j++;  /* sometimes we see spaces before the closing = */

            if (articlebuf[j]!='=')
                continue;  /* we want the exact heading */

            /* we have a winner! */
            hist[curhist].curoff=navnext;
        }
        target=0;
    }

    set_article_offset(hist[curhist].curoff);
    nextlinestate=render(curlinestate,0,0);

    /* get authoritative title name */
    rb->memcpy(hist[curhist].name,articlebuf+10,articlebuf[8]);
    hist[curhist].name[(int)(articlebuf[8])]='\0';

    mode=MODE_NORMAL;
    linkno=0;
    navcur=0;
    navnext=0;
    navline=0;
    histviewoff=0;
    bookviewoff=0;


    int button;
    while (!wrquit)
    {
        button = rb->button_get(true);

        if (rb->default_event_handler_ex(button, viewer_exit, NULL)==
            SYS_USB_CONNECTED)
            return PLUGIN_USB_CONNECTED;

#ifdef HAVE_TOUCHSCREEN
        if (button & BUTTON_TOUCHSCREEN)
        {
            static int prevY = 0;
            short x, y;
            int fontheight = rb->font_get(FONT_UI)->height;
            x = rb->button_get_data() >> 16;
            y = rb->button_get_data() & 0xFFFF;

            if(button & BUTTON_REL)
                prevY = 0;
            else
            {
                if (prevY != 0)
                {
                    if (mode == MODE_NORMAL)
                    {
                        int pageDelta = (y - prevY) / fontheight;
                        advance_scrollback(-pageDelta, 1);
                    }
                }
                prevY = y;
            }
        }
#endif

        switch(button)
        {
            case WIKIVIEWER_MENU:
                switch (mode)
                {
                    case MODE_BOOKVIEW:
                        save_bookmark(filename);
                    case MODE_HISTVIEW:
                    case MODE_LINK:
                    case MODE_NAVVIEW:
                        rb->lcd_clear_display();
                        mode=MODE_NORMAL;
                        nextlinestate=render(curlinestate,0,0);
                        break;
                    case MODE_NORMAL:
                    {
                        /* wrquit=true; */
                        MENUITEM_STRINGLIST(main_menu, "Wikiviewer Menu", NULL,
                                            "Save History",
                                            "Navigate",
                                            "Find Similar Article",
                                            "Find Article",
                                            "Clear History",
                                            "View History",
                                            "Show Playback Menu",
                                            "Bookmarks",
                                            "Add Bookmark",
                                            "Exit");

                        int selection = 0;
                        int lasthist;
                        switch (rb->do_menu(&main_menu, &selection, NULL, false))
                        {
                            case MENU_ATTACHED_USB:
                                return PLUGIN_USB_CONNECTED;

                            case 0:         /* save status */
                                save_status(filename);
                                break;

                            case 1:         /* navigate */
                                mode=MODE_NAVVIEW;
                                navcur=0;
                                navnext=render_outline(navcur,0);
                                navline=0;
                                break;

                            case 2:         /* find similar article */
                                lasthist=curhist;
                                prompt=1;
                                dofind=1;

                                advance_history(1);
                                rb->strcpy(hist[curhist].name,hist[lasthist].name);
                                hist[curhist].curoff=0;

                                goto loadnewarticle;
                                break;

                            case 3:         /* find article (blank name) */
                                lasthist=curhist;
                                prompt=1;
                                dofind=1;

                                advance_history(1);
                                rb->memset(hist[curhist].name,0,ARTICLENAMEBUF_LENGTH);
                                hist[curhist].curoff=0;

                                goto loadnewarticle;
                                break;

                            case 4:         /* clear hist */
                                reset_history();
                                rb->memset(hist[curhist].name,0,ARTICLENAMEBUF_LENGTH);
                                hist[curhist].curoff=0;
                                prompt=1;
                                dofind=1;
                                goto loadnewarticle;
                                break;

                            case 5:         /* view hist */
                                mode=MODE_HISTVIEW;
                                histviewoff=0;
                                render_history(histviewoff);
                                break;

                            case 6:         /* playback control */
                                playback_control(NULL);
                                break;

                            case 7:         /* Bookmarks */
                                mode=MODE_BOOKVIEW;
                                render_bookmarks(0);
                                break;

                            case 8:         /* Add Bookmark */
                                add_bookmark();
                                save_bookmark(filename);
                                break;

                            case 9:         /* exit */
                                wrquit=true;
                                break;
                        }     /* end menu switch */

                        if ((mode==MODE_NORMAL) && !wrquit)
                        {
                            rb->lcd_clear_display();
                            nextlinestate=render(curlinestate,0,0);
                        }
                    }     /* end menu block */
                }     /* end mode switch */
                break;

            case WIKIVIEWER_SELECT:
                switch (mode)
                {
                    case MODE_NAVVIEW:
                        set_article_offset(navcur);
                        nextlinestate=render(curlinestate,0,0);
                        mode=MODE_NORMAL;
                        break;

                    case MODE_NORMAL:
                        /* enter link select mode */
                        mode=MODE_LINK;
                        linkno=1;
                        nextlinestate=render(curlinestate,0,linkno);
                        if (lastlinkcount==0) mode=MODE_NORMAL;

                        break;

                    case MODE_LINK:
                        /* load a new article */
                        advance_history(1);
                        readlink(curlinestate,linkno,hist[curhist].name,ARTICLENAMEBUF_LENGTH);
                        if ((target=rb->strrchr(hist[curhist].name,'#')))
                        {
                            /* cut the target name off the end of the string */
                            target[0]='\0';
                            target++;     /* to point at the name of the target */
                        }

                        hist[curhist].curoff=0;
                        dofind=1;
                        goto loadnewarticle;        /* ~line 1165 */

                    case MODE_HISTVIEW:
                        /* jump back */
                        if (histviewoff>0)
                        {
                            int i;
                            for (i=0; i<histviewoff; i++)
                                advance_history(-1);
                            dofind=0;
                            mode=MODE_NORMAL;
                            goto loadnewarticle;        /* ~line 1165 */
                        }

                        nextlinestate=render(curlinestate,0,0);
                        mode=MODE_NORMAL;
                        break;

                    case MODE_BOOKVIEW:
                        if ((bookdepth-bookviewoff)<0)
                            rb->splash(HZ*2, "error");
                        else
                        {
                            save_bookmark(filename);
                            int booki;
                            booki=bookdepth-bookviewoff-1;
                            curhist++;
                            histdepth++;
                            rb->strcpy(hist[curhist].name,book[booki].name);
                            hist[curhist].curoff=book[booki].curoff;
                            hist[curhist].res_lo=book[booki].res_lo;
                            hist[curhist].res_hi=book[booki].res_hi;
                            dofind=0;
                            mode=MODE_NORMAL;
                            goto loadnewarticle;        /* ~line 1165 */
                        }

                        nextlinestate=render(curlinestate,0,0);
                        mode=MODE_NORMAL;
                        break;

                    default:
                        break;
                }     /* end mode switch */
                break;

#ifdef WIKIVIEWER_STOP_RECORD
            case WIKIVIEWER_STOP_RECORD:
                switch (mode)
                {
                    case MODE_NORMAL:              /*reset recording flag*/
                        record_stop_flag=true;
                        break;
                    default:
                        break;
                }     /* end mode switch */
                break;
#endif

            case WIKIVIEWER_FWD:
            case WIKIVIEWER_FWD_REPEAT:
#ifdef WIKIVIEWER_FWD2
            case WIKIVIEWER_FWD2:
            case WIKIVIEWER_FWD2_REPEAT:
#endif
                switch (mode)
                {
                    case MODE_NAVVIEW:
                        if (navnext!=navcur)
                            navline++;

                        navcur=navnext;
                        navnext=render_outline(navcur,0);
                        break;

                    case MODE_LINK:
                        /* select next link */
                        if (linkno<lastlinkcount)
                        {
                            linkno++;
                            nextlinestate=render(curlinestate,0,linkno);
                        }

                        break;

                    case MODE_NORMAL:
#ifdef WIKIVIEWER_FWD2
                        if (button==WIKIVIEWER_FWD2)
                        {
                            /* enter link select mode */
                            mode=MODE_LINK;
                            linkno=1;
                            nextlinestate=render(curlinestate,0,linkno);
                            if (lastlinkcount==0)
                                mode=MODE_NORMAL;

                            break;
                        }
                        else
#endif
                        {                               /* scroll down */
                            advance_scrollback(1,1);
                        }

                        break;

                    case MODE_HISTVIEW:
                        if (histviewoff<histdepth-1)
                        {
                            histviewoff++;
                            render_history(histviewoff);
                        }

                        break;

                    case MODE_BOOKVIEW:
                        if (bookviewoff<bookdepth-1)
                        {
                            bookviewoff++;
                            render_bookmarks(bookviewoff);
                        }

                        break;
                }     /* end mode switch */
                break;

            case WIKIVIEWER_BACK:
            case WIKIVIEWER_BACK_REPEAT:
#ifdef WIKIVIEWER_BACK2
            case WIKIVIEWER_BACK2:
            case WIKIVIEWER_BACK2_REPEAT:
#endif
                switch (mode)
                {
                    case MODE_NAVVIEW:
                    {
                        int i;
                        if (navline>0)
                        {
                            navline--;
                            navnext=0;
                            for (i=0; i<navline; i++)
                            {
                                navcur=navnext;
                                navnext=render_outline(navcur,1);
                            }
                            navcur=navnext;
                            navnext=render_outline(navcur,0);
                        }
                    }
                    break;

                    case MODE_LINK:
                        /* select previous link */
                        if (linkno>1)
                        {
                            linkno--;
                            nextlinestate=render(curlinestate,0,linkno);
                        }

                        break;

                    case MODE_NORMAL:
#ifdef WIKIVIEWER_BACK2
                        if (button==WIKIVIEWER_BACK2)         /*scroll up page*/
                            advance_scrollback((1-(LCD_HEIGHT/fontheight)),1);
                        else
#endif
                        {                               /* scroll up */
                            advance_scrollback(-1,1);
                        }

                        break;

                    case MODE_HISTVIEW:
                        if (histviewoff>0)
                        {
                            histviewoff--;
                            render_history(histviewoff);
                        }

                        break;

                    case MODE_BOOKVIEW:
                        if (bookviewoff>0)
                        {
                            bookviewoff--;
                            render_bookmarks(bookviewoff);
                        }

                        break;
                }     /* end mode switch */
                break;

            case WIKIVIEWER_PREV:     /* go back */
                switch (mode)
                {
                    case MODE_NORMAL:
                        if (histdepth>1)
                        {
                            advance_history(-1);
                            dofind=0;
                            goto loadnewarticle;        /* ~line 1165 */
                        }

                        break;

                    case MODE_BOOKVIEW:
                        remove_bookmark(bookviewoff);
                        bookviewoff=0;
                        render_bookmarks(bookviewoff);
                        break;

                    default:
                        break;
                }     /* end mode switch */
                break;

            case WIKIVIEWER_NEXT:     /* go forward */
                switch (mode)
                {
                    case MODE_NORMAL:
                        if (hist[(curhist+1)].curoff>0)
                        {
                            advance_history(1);
                            dofind=0;
                            bookviewoff=0;
                            goto loadnewarticle;        /* ~line 1165 */
                        }

                        break;

                    case MODE_BOOKVIEW:
                        if (load_bookmarks(filename))
                            render_bookmarks(bookviewoff);

                        break;

                    default:
                        break;
                }     /* end mode switch */
                break;

#ifdef WIKIVIEWER_RECORD
            case WIKIVIEWER_RECORD:                            /*record line*/
            case WIKIVIEWER_RECORD_REPEAT:
                switch (mode)
                {
                    case MODE_NORMAL:
                        record_flag=1;
                        advance_scrollback(1,1);
                        break;
                    case MODE_BOOKVIEW:
                        save_bookmark(filename);
                        break;
                    default:
                        break;
                }     /* end mode switch */
                break;
#endif

            default:
                break;
        } /* end button switch */
    } /* end main loop */
    return true;
}

static bool check_dir(char *folder)
{
    DIR *dir = rb->opendir(folder);
    if (!dir && rb->strcmp(folder, "/"))
    {
        int rc = rb->mkdir(folder);
        if(rc < 0)
            return false;

        return true;
    }

    rb->closedir(dir);
    return true;
}

enum plugin_status plugin_start(const void* file)
{
    rb->backlight_set_timeout(0);  /*Turn off backlight timeout*/
    check_dir("/wiki");
    if (!file)
        return PLUGIN_ERROR;

    rb->strlcpy(filename,file,sizeof(filename));
    filename[rb->strlen(filename)-4]='\0';

    rb->lcd_set_drawmode(DRMODE_SOLID);

    if (!viewer_init())
        return PLUGIN_ERROR;

    viewer_exit(NULL);
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
    /*Turn on backlight timeout*/
    return PLUGIN_OK;
}
