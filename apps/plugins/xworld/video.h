/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Franklin Wei, Benjamin Brown
 * Copyright (C) 2004 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#ifndef __VIDEO_H__
#define __VIDEO_H__

#include "intern.h"

struct StrEntry {
    uint16_t id;
    char *str;
};
#define MAX_POINTS 50
struct Polygon {

    uint16_t bbw, bbh;
    uint8_t numPoints;
    struct Point points[MAX_POINTS];

};
void polygon_readVertices(struct Polygon*, const uint8_t *p, uint16_t zoom);

struct Resource;
struct Serializer;
struct System;

// This is used to detect the end of  _stringsTableEng and _stringsTableDemo
#define END_OF_STRING_DICTIONARY 0xFFFF

// Special value when no palette change is necessary
#define NO_PALETTE_CHANGE_REQUESTED 0xFF

/* 320x200 pixels, with 2 pixels/byte */
#define VID_PAGE_SIZE ( 320 * 200 / 2 )

struct Video {

    /* FW: static const uint8_t _font[];*/
    /* FW: moved to video_data.c */
    struct Resource *res;
    struct System *sys;



    uint8_t paletteIdRequested, currentPaletteId;
    uint8_t *_pagePtrs[4];
    uint8_t *page_data;
    // I am almost sure that:
    // _curPagePtr1 is the backbuffer
    // _curPagePtr2 is the frontbuffer
    // _curPagePtr3 is the background builder.
    uint8_t *_curPagePtr1, *_curPagePtr2, *_curPagePtr3;

    struct Polygon polygon;
    int16_t _hliney;

    //Precomputer division lookup table
    uint16_t _interpTable[0x400];

    struct Ptr _pData;
    uint8_t *_dataBuf;


};

typedef void (*drawLine)(struct Video*, int16_t x1, int16_t x2, uint8_t col);

//Video(Resource *res, System *stub);
void video_create(struct Video*, struct Resource*, struct System*);
void video_init(struct Video* v);

void video_setDataBuffer(struct Video* v, uint8_t *dataBuf, uint16_t offset);
void video_readAndDrawPolygon(struct Video* v, uint8_t color, uint16_t zoom, const struct Point *pt);
void video_fillPolygon(struct Video* v, uint16_t color, uint16_t zoom, const struct Point *pt);
void video_readAndDrawPolygonHierarchy(struct Video* v, uint16_t zoom, const struct Point *pt);
int32_t video_calcStep(struct Video* v, const struct Point *p1, const struct Point *p2, uint16_t *dy);

void video_drawString(struct Video* v, uint8_t color, uint16_t x, uint16_t y, uint16_t strId);
void video_drawChar(struct Video* v, uint8_t c, uint16_t x, uint16_t y, uint8_t color, uint8_t *buf);
void video_drawPoint(struct Video* v, uint8_t color, int16_t x, int16_t y);
void video_drawLineBlend(struct Video* v, int16_t x1, int16_t x2, uint8_t color);
void video_drawLineN(struct Video* v, int16_t x1, int16_t x2, uint8_t color);
void video_drawLineP(struct Video* v, int16_t x1, int16_t x2, uint8_t color);
uint8_t *video_getPagePtr(struct Video* v, uint8_t page);
void video_changePagePtr1(struct Video* v, uint8_t page);
void video_fillPage(struct Video* v, uint8_t page, uint8_t color);
void video_copyPage(struct Video* v, uint8_t src, uint8_t dst, int16_t vscroll);
void video_copyPagePtr(struct Video* v, const uint8_t *src);
uint8_t *video_allocPage(struct Video* v);
void video_changePal(struct Video* v, uint8_t pal);
void video_updateDisplay(struct Video* v, uint8_t page);

void video_saveOrLoad(struct Video* v, struct Serializer *ser);

#define TRACE_PALETTE 0
#define TRACE_FRAMEBUFFER 0
#if TRACE_FRAMEBUFFER
void video_dumpFrameBuffer(struct Video* v, uint8_t *src, uint8_t *dst, int x, int y);
void video_dumpFrameBuffers(struct Video* v, char* comment);

#endif

#define TRACE_BG_BUFFER 0
#if TRACE_BG_BUFFER
void video_dumpBackGroundBuffer(struct Video* v);
#endif

#endif
