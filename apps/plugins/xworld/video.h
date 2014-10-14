/* Raw - Another World Interpreter
 * Copyright (C) 2004 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __VIDEO_H__
#define __VIDEO_H__

#include "intern.h"

struct StrEntry {
    uint16_t id;
    const char *str;
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

typedef void (Video::*drawLine)(int16_t x1, int16_t x2, uint8_t col);
struct Video {

    /* FW: static const uint8_t _font[];*/
    /* FW: moved to video_data.c */
    Resource *res;
    System *sys;



    uint8_t paletteIdRequested, currentPaletteId;
    uint8_t *_pagePtrs[4];

    // I am almost sure that:
    // _curPagePtr1 is the backbuffer
    // _curPagePtr2 is the frontbuffer
    // _curPagePtr3 is the background builder.
    uint8_t *_curPagePtr1, *_curPagePtr2, *_curPagePtr3;

    Polygon polygon;
    int16_t _hliney;

    //Precomputer division lookup table
    uint16_t _interpTable[0x400];

    Ptr _pData;
    uint8_t *_dataBuf;


};


//Video(Resource *res, System *stub);
void video_create(struct Video*, struct Resource*, struct System*);
void video_init(struct Video* v);

void video_setDataBuffer(struct Video* v, uint8_t *dataBuf, uint16_t offset);
void video_readAndDrawPolygon(struct Video* v, uint8_t color, uint16_t zoom, const Point &pt);
void video_fillPolygon(struct Video* v, uint16_t color, uint16_t zoom, const Point *pt);
void video_readAndDrawPolygonHierarchy(struct Video* v, uint16_t zoom, const Point *pt);
int32_t video_calcStep(struct Video* v, const Point *p1, const Point *p2, uint16_t *dy);

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

void video_saveOrLoad(struct Video* v, Serializer &ser);

#define TRACE_PALETTE 0
#define TRACE_FRAMEBUFFER 0
#if TRACE_FRAMEBUFFER
void video_dumpFrameBuffer(struct Video* v, uint8_t *src,uint8_t *dst, int x,int y);
void video_dumpFrameBuffers(struct Video* v, char* comment);

#endif

#define TRACE_BG_BUFFER 0
#if TRACE_BG_BUFFER
void video_dumpBackGroundBuffer(struct Video* v);
#endif

#endif
