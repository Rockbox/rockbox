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

#include "video.h"
#include "video_data.h"
#include "resource.h"
#include "serializer.h"
#include "sys.h"
#include "file.h"

void polygon_readVertices(struct Polygon* g, const uint8_t *p, uint16_t zoom) {
    g->bbw = (*p++) * zoom / 64;
    g->bbh = (*p++) * zoom / 64;
    g->numPoints = *p++;
    assert((g->numPoints & 1) == 0 && g->numPoints < MAX_POINTS);

    //Read all points, directly from bytecode segment
    for (int i = 0; i < g->numPoints; ++i) {
        struct Point *pt = &g->points[i];
        pt->x = (*p++) * zoom / 64;
        pt->y = (*p++) * zoom / 64;
    }
}

void video_create(struct Video* v, struct Resource* res, struct System* sys)
{
    v->res = res;
    v->sys = sys;
}

void video_init(struct Video* v) {
    v->paletteIdRequested = NO_PALETTE_CHANGE_REQUESTED;
    v->page_data = v->res->_memPtrStart + MEM_BLOCK_SIZE;

    rb->memset(v->page_data, 0, 4 * VID_PAGE_SIZE);

    for (int i = 0; i < 4; ++i) {
        v->_pagePtrs[i] = v->page_data + i * VID_PAGE_SIZE;
    }

    v->_curPagePtr3 = video_getPagePtr(v, 1);
    v->_curPagePtr2 = video_getPagePtr(v, 2);

    video_changePagePtr1(v, 0xFE);

    v->_interpTable[0] = 0x4000;

    for (int i = 1; i < 0x400; ++i) {
        v->_interpTable[i] = 0x4000 / i;
    }
}

void video_setDataBuffer(struct Video* v, uint8_t *dataBuf, uint16_t offset) {

    v->_dataBuf = dataBuf;
    v->_pData.pc = dataBuf + offset;
}


/*  A shape can be given in two different ways:

    - A list of screenspace vertices.
    - A list of objectspace vertices, based on a delta from the first vertex.

    This is a recursive function. */
void video_readAndDrawPolygon(struct Video* v, uint8_t color, uint16_t zoom, const struct Point *pt) {

    uint8_t i = scriptPtr_fetchByte(&v->_pData);

    //This is
    if (i >= 0xC0) {    // 0xc0 = 192

        // WTF ?
        if (color & 0x80) {   //0x80 = 128 (1000 0000)
            color = i & 0x3F; //0x3F =  63 (0011 1111)
        }

        // pc is misleading here since we are not reading bytecode but only
        // vertices informations.
        polygon_readVertices(&v->polygon, v->_pData.pc, zoom);

        video_fillPolygon(v, color, zoom, pt);



    } else {
        i &= 0x3F;  //0x3F = 63
        if (i == 1) {
            warning("video_readAndDrawPolygon() ec=0x%X (i != 2)", 0xF80);
        } else if (i == 2) {
            video_readAndDrawPolygonHierarchy(v, zoom, pt);

        } else {
            warning("video_readAndDrawPolygon() ec=0x%X (i != 2)", 0xFBB);
        }
    }



}

void video_fillPolygon(struct Video* v, uint16_t color, uint16_t zoom, const struct Point *pt) {

    (void) zoom;

    if (v->polygon.bbw == 0 && v->polygon.bbh == 1 && v->polygon.numPoints == 4) {
        video_drawPoint(v, color, pt->x, pt->y);

        return;
    }

    int16_t x1 = pt->x - v->polygon.bbw / 2;
    int16_t x2 = pt->x + v->polygon.bbw / 2;
    int16_t y1 = pt->y - v->polygon.bbh / 2;
    int16_t y2 = pt->y + v->polygon.bbh / 2;

    if (x1 > 319 || x2 < 0 || y1 > 199 || y2 < 0)
        return;

    v->_hliney = y1;

    uint16_t i, j;
    i = 0;
    j = v->polygon.numPoints - 1;

    x2 = v->polygon.points[i].x + x1;
    x1 = v->polygon.points[j].x + x1;

    ++i;
    --j;

    drawLine drawFct;
    if (color < 0x10) {
        drawFct = &video_drawLineN;
    } else if (color > 0x10) {
        drawFct = &video_drawLineP;
    } else {
        drawFct = &video_drawLineBlend;
    }

    uint32_t cpt1 = x1 << 16;
    uint32_t cpt2 = x2 << 16;

    while (1) {
        v->polygon.numPoints -= 2;
        if (v->polygon.numPoints == 0) {
#if TRACE_FRAMEBUFFER
            video_dumpFrameBuffers(v, "fillPolygonEnd");
#endif
#if TRACE_BG_BUFFER
            video_dumpBackGroundBuffer(v);
#endif
            break;
        }
        uint16_t h;
        int32_t step1 = video_calcStep(v, &v->polygon.points[j + 1], &v->polygon.points[j], &h);
        int32_t step2 = video_calcStep(v, &v->polygon.points[i - 1], &v->polygon.points[i], &h);

        ++i;
        --j;

        cpt1 = (cpt1 & 0xFFFF0000) | 0x7FFF;
        cpt2 = (cpt2 & 0xFFFF0000) | 0x8000;

        if (h == 0) {
            cpt1 += step1;
            cpt2 += step2;
        } else {
            for (; h != 0; --h) {
                if (v->_hliney >= 0) {
                    x1 = cpt1 >> 16;
                    x2 = cpt2 >> 16;
                    if (x1 <= 319 && x2 >= 0) {
                        if (x1 < 0) x1 = 0;
                        if (x2 > 319) x2 = 319;
                        (*drawFct)(v, x1, x2, color);
                    }
                }
                cpt1 += step1;
                cpt2 += step2;
                ++v->_hliney;
                if (v->_hliney > 199) return;
            }
        }

#if TRACE_FRAMEBUFFER
        video_dumpFrameBuffers(v, "fillPolygonChild");
#endif
#if TRACE_BG_BUFFER

        video_dumpBackGroundBuffer(v);
#endif
    }
}

/*
  What is read from the bytecode is not a pure screnspace polygon but a polygonspace polygon.

*/
void video_readAndDrawPolygonHierarchy(struct Video* v, uint16_t zoom, const struct Point *pgc) {

    struct Point pt = *pgc;
    pt.x -= scriptPtr_fetchByte(&v->_pData) * zoom / 64;
    pt.y -= scriptPtr_fetchByte(&v->_pData) * zoom / 64;

    int16_t childs = scriptPtr_fetchByte(&v->_pData);
    debug(DBG_VIDEO, "video_readAndDrawPolygonHierarchy childs=%d", childs);

    for ( ; childs >= 0; --childs) {

        uint16_t off = scriptPtr_fetchWord(&v->_pData);

        struct Point po;
        po = pt;
        po.x += scriptPtr_fetchByte(&v->_pData) * zoom / 64;
        po.y += scriptPtr_fetchByte(&v->_pData) * zoom / 64;

        uint16_t color = 0xFF;
        uint16_t _bp = off;
        off &= 0x7FFF;

        if (_bp & 0x8000) {
            color = *v->_pData.pc & 0x7F;
            v->_pData.pc += 2;
        }

        uint8_t *bak = v->_pData.pc;
        v->_pData.pc = v->_dataBuf + off * 2;


        video_readAndDrawPolygon(v, color, zoom, &po);


        v->_pData.pc = bak;
    }


}

int32_t video_calcStep(struct Video* v, const struct Point *p1, const struct Point *p2, uint16_t *dy) {
    (void) v;
    *dy = p2->y - p1->y;
    return (p2->x - p1->x) * v->_interpTable[*dy] * 4;
}

void video_drawString(struct Video* v, uint8_t color, uint16_t x, uint16_t y, uint16_t stringId) {

    const struct StrEntry *se = video_stringsTableEng;

    //Search for the location where the string is located.
    while (se->id != END_OF_STRING_DICTIONARY && se->id != stringId)
        ++se;

    debug(DBG_VIDEO, "video_drawString(%d, %d, %d, '%s')", color, x, y, se->str);

    //Not found
    if (se->id == END_OF_STRING_DICTIONARY)
        return;


    //Used if the string contains a return carriage.
    uint16_t xOrigin = x;
    int len = rb->strlen(se->str);
    for (int i = 0; i < len; ++i) {

        if (se->str[i] == '\n') {
            y += 8;
            x = xOrigin;
            continue;
        }

        video_drawChar(v, se->str[i], x, y, color, v->_curPagePtr1);
        x++;

    }
}

void video_drawChar(struct Video* v, uint8_t character, uint16_t x, uint16_t y, uint8_t color, uint8_t *buf) {
    (void) v;
    if (x <= 39 && y <= 192) {

        /* each character is 8x8 */
        const uint8_t *ft = video_font + (character - ' ') * 8;

        /* x is multiplied by 4 and not 8 because there are two pixels per byte */
        uint8_t *p = buf + x * 4 + y * 160;

        for (int j = 0; j < 8; ++j) {
            uint8_t ch = *(ft + j);
            for (int i = 0; i < 4; ++i) {
                uint8_t b = *(p + i);
                uint8_t cmask = 0xFF;
                uint8_t colb = 0;
                if (ch & 0x80) {
                    colb |= color << 4;
                    cmask &= 0x0F;
                }
                ch <<= 1;
                if (ch & 0x80) {
                    colb |= color;
                    cmask &= 0xF0;
                }
                ch <<= 1;
                *(p + i) = (b & cmask) | colb;
            }
            /* skip to the next line (320 pixels = 160 bytes) */
            p += 160;
        }
    }
}

void video_drawPoint(struct Video* v, uint8_t color, int16_t x, int16_t y) {
    debug(DBG_VIDEO, "drawPoint(%d, %d, %d)", color, x, y);
    if (x >= 0 && x <= 319 && y >= 0 && y <= 199) {
        uint16_t off = y * 160 + x / 2;

        uint8_t cmasko, cmaskn;
        if (x & 1) {
            cmaskn = 0x0F;
            cmasko = 0xF0;
        } else {
            cmaskn = 0xF0;
            cmasko = 0x0F;
        }

        uint8_t colb = (color << 4) | color;
        if (color == 0x10) {
            cmaskn &= 0x88;
            cmasko = ~cmaskn;
            colb = 0x88;
        } else if (color == 0x11) {
            colb = *(v->_pagePtrs[0] + off);
        }
        uint8_t b = *(v->_curPagePtr1 + off);
        *(v->_curPagePtr1 + off) = (b & cmasko) | (colb & cmaskn);
    }
}

/* Blend a line in the current framebuffer (v->_curPagePtr1)
 */
void video_drawLineBlend(struct Video* v, int16_t x1, int16_t x2, uint8_t color) {
    /* silence warnings without XWORLD_DEBUG */
    (void) color;
    debug(DBG_VIDEO, "drawLineBlend(%d, %d, %d)", x1, x2, color);
    int16_t xmax = MAX(x1, x2);
    int16_t xmin = MIN(x1, x2);
    uint8_t *p = v->_curPagePtr1 + v->_hliney * 160 + xmin / 2;

    uint16_t w = xmax / 2 - xmin / 2 + 1;
    uint8_t cmaske = 0;
    uint8_t cmasks = 0;
    if (xmin & 1) {
        --w;
        cmasks = 0xF7;
    }
    if (!(xmax & 1)) {
        --w;
        cmaske = 0x7F;
    }

    if (cmasks != 0) {
        *p = (*p & cmasks) | 0x08;
        ++p;
    }
    while (w--) {
        *p = (*p & 0x77) | 0x88;
        ++p;
    }
    if (cmaske != 0) {
        *p = (*p & cmaske) | 0x80;
        ++p;
    }


}

void video_drawLineN(struct Video* v, int16_t x1, int16_t x2, uint8_t color) {
    debug(DBG_VIDEO, "drawLineN(%d, %d, %d)", x1, x2, color);
    int16_t xmax = MAX(x1, x2);
    int16_t xmin = MIN(x1, x2);
    uint8_t *p = v->_curPagePtr1 + v->_hliney * 160 + xmin / 2;

    uint16_t w = xmax / 2 - xmin / 2 + 1;
    uint8_t cmaske = 0;
    uint8_t cmasks = 0;
    if (xmin & 1) {
        --w;
        cmasks = 0xF0;
    }
    if (!(xmax & 1)) {
        --w;
        cmaske = 0x0F;
    }

    uint8_t colb = ((color & 0xF) << 4) | (color & 0xF);
    if (cmasks != 0) {
        *p = (*p & cmasks) | (colb & 0x0F);
        ++p;
    }
    while (w--) {
        *p++ = colb;
    }
    if (cmaske != 0) {
        *p = (*p & cmaske) | (colb & 0xF0);
        ++p;
    }


}

void video_drawLineP(struct Video* v, int16_t x1, int16_t x2, uint8_t color) {
    /* silence warnings without XWORLD_DEBUG */
    (void) color;
    debug(DBG_VIDEO, "drawLineP(%d, %d, %d)", x1, x2, color);
    int16_t xmax = MAX(x1, x2);
    int16_t xmin = MIN(x1, x2);
    uint16_t off = v->_hliney * 160 + xmin / 2;
    uint8_t *p = v->_curPagePtr1 + off;
    uint8_t *q = v->_pagePtrs[0] + off;

    uint8_t w = xmax / 2 - xmin / 2 + 1;
    uint8_t cmaske = 0;
    uint8_t cmasks = 0;
    if (xmin & 1) {
        --w;
        cmasks = 0xF0;
    }
    if (!(xmax & 1)) {
        --w;
        cmaske = 0x0F;
    }

    if (cmasks != 0) {
        *p = (*p & cmasks) | (*q & 0x0F);
        ++p;
        ++q;
    }
    while (w--) {
        *p++ = *q++;
    }
    if (cmaske != 0) {
        *p = (*p & cmaske) | (*q & 0xF0);
        ++p;
        ++q;
    }

}

uint8_t *video_getPagePtr(struct Video* v, uint8_t page) {
    uint8_t *p;
    if (page <= 3) {
        p = v->_pagePtrs[page];
    } else {
        switch (page) {
        case 0xFF:
            p = v->_curPagePtr3;
            break;
        case 0xFE:
            p = v->_curPagePtr2;
            break;
        default:
            p = v->_pagePtrs[0]; // XXX check
            warning("video_getPagePtr() p != [0,1,2,3,0xFF,0xFE] == 0x%X", page);
            break;
        }
    }
    return p;
}



void video_changePagePtr1(struct Video* v, uint8_t page) {
    debug(DBG_VIDEO, "video_changePagePtr1(%d)", page);
    v->_curPagePtr1 = video_getPagePtr(v, page);
}



void video_fillPage(struct Video* v, uint8_t pageId, uint8_t color) {
    debug(DBG_VIDEO, "video_fillPage(%d, %d)", pageId, color);
    uint8_t *p = video_getPagePtr(v, pageId);

    // Since a palette indice is coded on 4 bits, we need to duplicate the
    // clearing color to the upper part of the byte.
    uint8_t c = (color << 4) | color;

    rb->memset(p, c, VID_PAGE_SIZE);

#if TRACE_FRAMEBUFFER
    video_dumpFrameBuffers(v, "-fillPage");
#endif
#if TRACE_BG_BUFFER

    video_dumpBackGroundBuffer(v);
#endif
}





#if TRACE_FRAMEBUFFER
#define SCREENSHOT_BPP 3
int traceFrameBufferCounter = 0;
uint8_t allFrameBuffers[640 * 400 * SCREENSHOT_BPP];

#endif






/*  This opcode is used once the background of a scene has been drawn in one of the framebuffer:
    it is copied in the current framebuffer at the start of a new frame in order to improve performances. */
void video_copyPage(struct Video* v, uint8_t srcPageId, uint8_t dstPageId, int16_t vscroll) {

    debug(DBG_VIDEO, "video_copyPage(%d, %d)", srcPageId, dstPageId);

    if (srcPageId == dstPageId)
        return;

    uint8_t *p;
    uint8_t *q;

    if (srcPageId >= 0xFE || !((srcPageId &= 0xBF) & 0x80)) {
        p = video_getPagePtr(v, srcPageId);
        q = video_getPagePtr(v, dstPageId);
        memcpy(q, p, VID_PAGE_SIZE);

    } else {
        p = video_getPagePtr(v, srcPageId & 3);
        q = video_getPagePtr(v, dstPageId);
        if (vscroll >= -199 && vscroll <= 199) {
            uint16_t h = 200;
            if (vscroll < 0) {
                h += vscroll;
                p += -vscroll * 160;
            } else {
                h -= vscroll;
                q += vscroll * 160;
            }
            memcpy(q, p, h * 160);
        }
    }


#if TRACE_FRAMEBUFFER
    char name[256];
    rb->memset(name, 0, sizeof(name));
    sprintf(name, "copyPage_0x%X_to_0x%X", (p - v->_pagePtrs[0]) / VID_PAGE_SIZE, (q - v->_pagePtrs[0]) / VID_PAGE_SIZE);
    dumpFrameBuffers(name);
#endif
}




void video_copyPagePtr(struct Video* v, const uint8_t *src) {
    debug(DBG_VIDEO, "video_copyPagePtr()");
    uint8_t *dst = v->_pagePtrs[0];
    int h = 200;
    while (h--) {
        int w = 40;
        while (w--) {
            uint8_t p[] = {
                *(src + 8000 * 3),
                *(src + 8000 * 2),
                *(src + 8000 * 1),
                *(src + 8000 * 0)
            };
            for(int j = 0; j < 4; ++j) {
                uint8_t acc = 0;
                for (int i = 0; i < 8; ++i) {
                    acc <<= 1;
                    acc |= (p[i & 3] & 0x80) ? 1 : 0;
                    p[i & 3] <<= 1;
                }
                *dst++ = acc;
            }
            ++src;
        }
    }


}

/*
  uint8_t *video_allocPage() {
  uint8_t *buf = (uint8_t *)malloc(VID_PAGE_SIZE);
  rb->memset(buf, 0, VID_PAGE_SIZE);
  return buf;
  }
*/



#if TRACE_FRAMEBUFFER
int dumpPaletteCursor = 0;
#endif

/*
  Note: The palettes set used to be allocated on the stack but I moved it to
  the heap so I could dump the four framebuffer and follow how
  frames are generated.
*/
uint8_t pal[NUM_COLORS * 3]; //3 = BYTES_PER_PIXEL
void video_changePal(struct Video* v, uint8_t palNum) {
    debug(DBG_VIDEO, "video_changePal(v=0x%08x, palNum=%d",  v, palNum);
    if (palNum >= 32)
        return;

    uint8_t *p = v->res->segPalettes + palNum * 32; //colors are coded on 2bytes (565) for 16 colors = 32
    debug(DBG_VIDEO, "segPalettes: 0x%08x", v->res->segPalettes);
    // Moved to the heap, legacy code used to allocate the palette
    // on the stack.
    //uint8_t pal[NUM_COLORS * 3]; //3 = BYTES_PER_PIXEL

    for (int i = 0; i < NUM_COLORS; ++i)
    {
        debug(DBG_VIDEO, "i: %d", i);
        debug(DBG_VIDEO, "p: 0x%08x", p);
        uint8_t c1 = *(p + 0);
        uint8_t c2 = *(p + 1);
        p += 2;
        pal[i * 3 + 0] = ((c1 & 0x0F) << 2) | ((c1 & 0x0F) >> 2); // r
        pal[i * 3 + 1] = ((c2 & 0xF0) >> 2) | ((c2 & 0xF0) >> 6); // g
        pal[i * 3 + 2] = ((c2 & 0x0F) >> 2) | ((c2 & 0x0F) << 2); // b
    }

    sys_setPalette(v->sys, 0, NUM_COLORS, pal);
    v->currentPaletteId = palNum;


#if TRACE_PALETTE
    printf("\nuint8_t dumpPalette[48] = {\n");
    for (int i = 0; i < NUM_COLORS; ++i)
    {
        printf("0x%X,0x%X,0x%X,", pal[i * 3 + 0], pal[i * 3 + 1], pal[i * 3 + 2]);
    }
    printf("\n};\n");
#endif


#if TRACE_FRAMEBUFFER
    dumpPaletteCursor++;
#endif
}

void video_updateDisplay(struct Video* v, uint8_t pageId) {

    debug(DBG_VIDEO, "video_updateDisplay(%d)", pageId);

    if (pageId != 0xFE) {
        if (pageId == 0xFF) {
            /* swap ptrs 2 and 3 */
            uint8_t* temp = v->_curPagePtr3;
            v->_curPagePtr3 = v->_curPagePtr2;
            v->_curPagePtr2 = temp;
        } else {
            v->_curPagePtr2 = video_getPagePtr(v, pageId);
        }
    }

    //Check if we need to change the palette
    if (v->paletteIdRequested != NO_PALETTE_CHANGE_REQUESTED) {
        video_changePal(v, v->paletteIdRequested);
        v->paletteIdRequested = NO_PALETTE_CHANGE_REQUESTED;
    }

    //Q: Why 160 ?
    //A: Because one byte gives two palette indices so
    //   we only need to move 320/2 per line.
    sys_copyRect(v->sys, 0, 0, 320, 200, v->_curPagePtr2, 160);

#if TRACE_FRAMEBUFFER
    dumpFrameBuffer(v->_curPagePtr2, allFrameBuffers, 320, 200);
#endif
}

void video_saveOrLoad(struct Video* v, struct Serializer *ser) {
    uint8_t mask = 0;
    if (ser->_mode == SM_SAVE) {
        for (int i = 0; i < 4; ++i) {
            if (v->_pagePtrs[i] == v->_curPagePtr1)
                mask |= i << 4;
            if (v->_pagePtrs[i] == v->_curPagePtr2)
                mask |= i << 2;
            if (v->_pagePtrs[i] == v->_curPagePtr3)
                mask |= i << 0;
        }
    }
    struct Entry entries[] = {
        SE_INT(&v->currentPaletteId, SES_INT8, VER(1)),
        SE_INT(&v->paletteIdRequested, SES_INT8, VER(1)),
        SE_INT(&mask, SES_INT8, VER(1)),
        SE_ARRAY(v->_pagePtrs[0], VID_PAGE_SIZE, SES_INT8, VER(1)),
        SE_ARRAY(v->_pagePtrs[1], VID_PAGE_SIZE, SES_INT8, VER(1)),
        SE_ARRAY(v->_pagePtrs[2], VID_PAGE_SIZE, SES_INT8, VER(1)),
        SE_ARRAY(v->_pagePtrs[3], VID_PAGE_SIZE, SES_INT8, VER(1)),
        SE_END()
    };
    ser_saveOrLoadEntries(ser, entries);

    if (ser->_mode == SM_LOAD) {
        v->_curPagePtr1 = v->_pagePtrs[(mask >> 4) & 0x3];
        v->_curPagePtr2 = v->_pagePtrs[(mask >> 2) & 0x3];
        v->_curPagePtr3 = v->_pagePtrs[(mask >> 0) & 0x3];
        video_changePal(v, v->currentPaletteId);
    }
}



#if TRACE_FRAMEBUFFER


uint8_t allPalettesDump[][48] = {


    {
        0x4, 0x4, 0x4, 0x22, 0x0, 0x0, 0x4, 0x8, 0x11, 0x4, 0xC, 0x15, 0x8, 0x11, 0x19, 0xC, 0x15, 0x1D, 0x15, 0x1D, 0x26, 0x1D, 0x2A, 0x2E, 0x2E, 0x22, 0x0, 0x3F, 0x0, 0x0, 0x33, 0x26, 0x0, 0x37, 0x2A, 0x0, 0x3B, 0x33, 0x0, 0x3F, 0x3B, 0x0, 0x3F, 0x3F, 0x1D, 0x3F, 0x3F, 0x2A,
    },

    {
        0x4, 0x4, 0x4, 0xC, 0xC, 0x11, 0x4, 0x8, 0x11, 0x4, 0xC, 0x15, 0x8, 0x11, 0x19, 0xC, 0x15, 0x1D, 0x15, 0x1D, 0x26, 0x1D, 0x2A, 0x2E, 0x2E, 0x22, 0x0, 0x15, 0x11, 0x11, 0x33, 0x26, 0x0, 0x37, 0x2A, 0x0, 0x3B, 0x33, 0x0, 0x3F, 0x3B, 0x0, 0x3F, 0x3F, 0x1D, 0x3F, 0x3F, 0x2A,
    }

    , {
        0x0, 0x0, 0x0, 0x22, 0x0, 0x0, 0x4, 0x8, 0x11, 0x4, 0xC, 0x15, 0x8, 0x11, 0x19, 0xC, 0x15, 0x1D, 0x15, 0x1D, 0x26, 0x1D, 0x2A, 0x2E, 0x1D, 0x1D, 0x1D, 0x15, 0x15, 0x15, 0xC, 0x8, 0xC, 0x11, 0x11, 0x15, 0x1D, 0x15, 0x15, 0x15, 0x0, 0x0, 0x0, 0x4, 0xC, 0x3F, 0x3F, 0x2A,
    }

    , {
        0x0, 0x0, 0x0, 0x22, 0x0, 0x0, 0x4, 0x8, 0x11, 0x4, 0xC, 0x15, 0x8, 0x11, 0x19, 0xC, 0x15, 0x1D, 0x15, 0x1D, 0x26, 0x1D, 0x2A, 0x2E, 0x1D, 0x1D, 0x1D, 0x15, 0x15, 0x15, 0xC, 0x8, 0xC, 0x11, 0x11, 0x15, 0x1D, 0x15, 0x15, 0x15, 0x0, 0x0, 0x0, 0x4, 0xC, 0x3F, 0x3F, 0x2A,
    }

    , {
        0x0, 0x0, 0x0, 0x1D, 0x0, 0x0, 0x4, 0x8, 0x11, 0x4, 0xC, 0x15, 0x8, 0x11, 0x19, 0xC, 0x15, 0x1D, 0x15, 0x1D, 0x26, 0x1D, 0x2A, 0x2E, 0x1D, 0x1D, 0x1D, 0x15, 0x15, 0x15, 0xC, 0x8, 0xC, 0x15, 0x11, 0x19, 0x1D, 0x15, 0x15, 0x15, 0x0, 0x0, 0x0, 0x4, 0xC, 0x3F, 0x3F, 0x2A,
    }

    , {
        0x0, 0x4, 0x8, 0x15, 0x1D, 0x1D, 0x0, 0x8, 0x11, 0x4, 0xC, 0x15, 0x8, 0x11, 0x19, 0xC, 0x15, 0x1D, 0xC, 0x19, 0x22, 0x11, 0x1D, 0x26, 0x8, 0x8, 0x8, 0x0, 0x0, 0x0, 0x2E, 0x2E, 0x2E, 0xC, 0xC, 0xC, 0x15, 0xC, 0x15, 0xC, 0x15, 0x15, 0x11, 0x19, 0x19, 0x1D, 0x26, 0x26,
    }

    , {
        0x0, 0x0, 0x0, 0x0, 0x4, 0xC, 0x4, 0x8, 0x11, 0x4, 0xC, 0x15, 0x8, 0x11, 0x19, 0xC, 0x15, 0x1D, 0x15, 0x1D, 0x26, 0x1D, 0x2A, 0x2E, 0x15, 0x0, 0x0, 0xC, 0xC, 0xC, 0xC, 0xC, 0xC, 0xC, 0x11, 0x11, 0x11, 0x11, 0x15, 0x26, 0x0, 0x0, 0x33, 0x26, 0x0, 0x3B, 0x33, 0x11,
    }

    , {
        0x0, 0x0, 0x0, 0x0, 0x4, 0xC, 0x0, 0x8, 0x11, 0x4, 0xC, 0x15, 0x8, 0x11, 0x19, 0xC, 0x15, 0x1D, 0x15, 0x1D, 0x26, 0x1D, 0x2A, 0x2E, 0x15, 0x0, 0x0, 0xC, 0xC, 0xC, 0xC, 0xC, 0xC, 0xC, 0x11, 0x11, 0x11, 0x11, 0x15, 0x26, 0x0, 0x0, 0x26, 0x15, 0x0, 0x26, 0x1D, 0x0,
    }

    , {
        0x0, 0x0, 0x0, 0x0, 0x4, 0xC, 0x0, 0x8, 0x11, 0x4, 0xC, 0x15, 0x8, 0x11, 0x19, 0xC, 0x15, 0x1D, 0x15, 0x1D, 0x26, 0x1D, 0x2A, 0x2E, 0x15, 0x0, 0x0, 0xC, 0xC, 0xC, 0xC, 0xC, 0xC, 0xC, 0x11, 0x11, 0x11, 0x11, 0x15, 0x26, 0x0, 0x0, 0x3F, 0x0, 0x0, 0x26, 0x1D, 0x0,
    }

    , {
        0x0, 0x0, 0x0, 0x8, 0x4, 0xC, 0x15, 0xC, 0x11, 0x1D, 0x11, 0x0, 0xC, 0x8, 0x11, 0x2E, 0x1D, 0x0, 0x37, 0x26, 0x8, 0x3F, 0x2E, 0x0, 0x0, 0x0, 0x0, 0x11, 0xC, 0x15, 0x26, 0x15, 0x0, 0x15, 0x11, 0x19, 0x1D, 0x15, 0x1D, 0x26, 0x19, 0x19, 0x0, 0x0, 0x0, 0x3F, 0x3F, 0x3F,
    }

    , {
        0x0, 0x0, 0x0, 0x8, 0x4, 0xC, 0x37, 0x1D, 0x1D, 0x3B, 0x2A, 0x22, 0x11, 0xC, 0x15, 0x2A, 0x0, 0x0, 0x33, 0x11, 0x0, 0x3F, 0x33, 0x1D, 0x3B, 0x19, 0x0, 0x11, 0x11, 0x19, 0x19, 0x15, 0x1D, 0x22, 0x19, 0x22, 0x2A, 0x1D, 0x26, 0x33, 0x22, 0x26, 0x37, 0x26, 0x22, 0x1D, 0x37, 0x3F,
    }

    , {
        0x0, 0x0, 0x0, 0x0, 0x0, 0x1D, 0x4, 0x8, 0xC, 0x2A, 0x1D, 0xC, 0x3F, 0x3B, 0x26, 0x3B, 0x2A, 0x11, 0x2A, 0x0, 0x0, 0x0, 0x11, 0x15, 0x2A, 0x1D, 0x26, 0xC, 0x8, 0xC, 0x8, 0x15, 0x1D, 0x37, 0x26, 0x22, 0x33, 0x11, 0x0, 0x2E, 0x1D, 0x19, 0x22, 0x0, 0x0, 0x3F, 0x33, 0x1D,
    }

    , {
        0x0, 0x0, 0x0, 0x0, 0x15, 0x0, 0x4, 0x8, 0xC, 0x0, 0xC, 0x11, 0x8, 0x11, 0x19, 0x0, 0x19, 0x22, 0x2A, 0x0, 0x0, 0x19, 0x15, 0x22, 0x2E, 0x26, 0x2E, 0xC, 0x8, 0xC, 0x0, 0x2E, 0x0, 0x37, 0x26, 0x22, 0x33, 0x11, 0x0, 0x2E, 0x1D, 0x19, 0x1D, 0x0, 0x0, 0x3F, 0x3F, 0x19,
    }

    , {
        0x0, 0x0, 0x0, 0x8, 0xC, 0x11, 0x11, 0x11, 0x15, 0x19, 0x15, 0x22, 0x26, 0x19, 0x2A, 0x2E, 0x26, 0x2E, 0x4, 0x4, 0xC, 0x0, 0xC, 0x15, 0x2A, 0x1D, 0x26, 0x0, 0x19, 0x0, 0x0, 0x2A, 0x0, 0x37, 0x26, 0x22, 0x0, 0x15, 0x1D, 0x37, 0x2E, 0x1D, 0x3F, 0x3F, 0x2E, 0x37, 0x37, 0x26,
    }

    , {
        0x0, 0x0, 0x0, 0x0, 0x15, 0x0, 0x4, 0x8, 0xC, 0x0, 0xC, 0x11, 0x8, 0x11, 0x19, 0x0, 0x19, 0x22, 0x2A, 0x0, 0x0, 0x19, 0x15, 0x22, 0x2E, 0x26, 0x2E, 0xC, 0x8, 0xC, 0x0, 0x2E, 0x0, 0x37, 0x26, 0x22, 0x33, 0x11, 0x0, 0x2E, 0x1D, 0x19, 0x1D, 0x0, 0x0, 0x3F, 0x3F, 0x19,
    }

    , {
        0x0, 0x0, 0x0, 0x0, 0xC, 0x0, 0x8, 0x0, 0x4, 0xC, 0x8, 0xC, 0x19, 0x11, 0x11, 0x3F, 0x3F, 0x19, 0x0, 0x1D, 0x0, 0x0, 0x3F, 0x0, 0x0, 0x4, 0x8, 0x11, 0x19, 0x11, 0x19, 0x0, 0x0, 0x2E, 0x0, 0x0, 0x3F, 0x0, 0x0, 0x37, 0x1D, 0x15, 0x0, 0x11, 0x11, 0x0, 0x3F, 0x2A,
    }

    , {
        0x0, 0x0, 0x0, 0x0, 0xC, 0x0, 0x0, 0x11, 0x0, 0x0, 0x19, 0x0, 0xC, 0x26, 0x0, 0x15, 0x2E, 0x0, 0x4, 0x8, 0x0, 0x26, 0x11, 0x0, 0x2E, 0x3F, 0x0, 0x0, 0x15, 0x0, 0xC, 0x1D, 0x0, 0x15, 0x2E, 0x0, 0x15, 0x37, 0x0, 0x1D, 0x3F, 0x0, 0xC, 0x1D, 0xC, 0x\
        1D, 0x26, 0x15,
    }
};



#include "png.h"
int GL_FCS_SaveAsSpecifiedPNG(char* path, uint8_t* pixels, int depth = 8, int format = PNG_COLOR_TYPE_RGB)
{
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_byte ** row_pointers = NULL;
    int status = -1;
    int bytePerPixel = 0;
    int y;

    int fd = open (path, "wb");
    if (fd < 0) {
        goto fopen_failed;
    }

    png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        goto png_create_write_struct_failed;
    }

    info_ptr = png_create_info_struct (png_ptr);
    if (info_ptr == NULL) {
        goto png_create_info_struct_failed;
    }

    if (setjmp (png_jmpbuf (png_ptr))) {
        goto png_failure;
    }

    /* Set image attributes. */

    png_set_IHDR (png_ptr,
                  info_ptr,
                  640,
                  400,
                  depth,
                  format,
                  PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_DEFAULT,
                  PNG_FILTER_TYPE_DEFAULT);

    if (format == PNG_COLOR_TYPE_GRAY   )
        bytePerPixel = depth / 8 * 1;
    else
        bytePerPixel = depth / 8 * 3;

    row_pointers = (png_byte **)png_malloc (png_ptr, 400 * sizeof (png_byte *));
    //for (y = vid.height-1; y >=0; --y)
    for (y = 0; y < 400; y++)
    {
        row_pointers[y] = (png_byte*)&pixels[640 * (400 - y) * bytePerPixel];
    }

    png_init_io (png_ptr, fp);
    png_set_rows (png_ptr, info_ptr, row_pointers);
    png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    //png_read_image (png_ptr, info_ptr);//

    status = 0;

    png_free (png_ptr, row_pointers);


png_failure:
png_create_info_struct_failed:
    png_destroy_write_struct (&png_ptr, &info_ptr);

png_create_write_struct_failed:
    fclose (fp);
fopen_failed:
    return status;
}

void writeLine(uint8_t *dst, uint8_t *src, int size)
{
    uint8_t* dumpPalette;

    if (!dumpPaletteCursor)
        dumpPalette = allPalettesDump[dumpPaletteCursor];
    else
        dumpPalette = pal;

    for( uint8_t twoPixels = 0 ; twoPixels < size ; twoPixels++)
    {
        int pixelIndex0 = (*src & 0xF0) >> 4;
        pixelIndex0 &= 0x10 - 1;

        int pixelIndex1 = (*src & 0xF);
        pixelIndex1 &= 0x10 - 1;

        //We need to write those two pixels
        dst[0] = dumpPalette[pixelIndex0 * 3] << 2 | dumpPalette[pixelIndex0 * 3];
        dst[1] = dumpPalette[pixelIndex0 * 3 + 1] << 2 | dumpPalette[pixelIndex0 * 3 + 1];
        dst[2] = dumpPalette[pixelIndex0 * 3 + 2] << 2 | dumpPalette[pixelIndex0 * 3 + 2];
        //dst[3] = 0xFF;
        dst += SCREENSHOT_BPP;

        dst[0] = dumpPalette[pixelIndex1 * 3] << 2 | dumpPalette[pixelIndex1 * 3];
        dst[1] = dumpPalette[pixelIndex1 * 3 + 1] << 2 | dumpPalette[pixelIndex1 * 3 + 1];
        dst[2] = dumpPalette[pixelIndex1 * 3 + 2] << 2 | dumpPalette[pixelIndex1 * 3 + 2];
        //dst[3] = 0xFF;
        dst += SCREENSHOT_BPP;

        src++;
    }
}

void video_dumpFrameBuffer(uint8_t *src, uint8_t *dst, int x, int y)
{

    for (int line = 199 ; line >= 0 ; line--)
    {
        writeLine(dst + x * SCREENSHOT_BPP + y * 640 * SCREENSHOT_BPP  , src + line * 160, 160);
        dst += 640 * SCREENSHOT_BPP;
    }
}

void video_dumpFrameBuffers(char* comment)
{

    if (!traceFrameBufferCounter)
    {
        rb->memset(allFrameBuffers, 0, sizeof(allFrameBuffers));
    }


    dumpFrameBuffer(v->_pagePtrs[1], allFrameBuffers, 0, 0);
    dumpFrameBuffer(v->_pagePtrs[0], allFrameBuffers, 0, 200);
    dumpFrameBuffer(v->_pagePtrs[2], allFrameBuffers, 320, 0);
    //dumpFrameBuffer(v->_pagePtrs[3],allFrameBuffers,320,200);


    //if (v->_curPagePtr1 == v->_pagePtrs[3])
    //

    /*
      uint8_t* offScreen = sys->getOffScreenFramebuffer();
      for(int i=0 ; i < 200 ; i++)
      writeLine(allFrameBuffers+320*3+640*i*3 + 200*640*3,  offScreen+320*i/2  ,  160);
    */


    int frameId = traceFrameBufferCounter++;
    //Write bitmap to disk.



    // Filling TGA header information
    /*
      char path[256];
      sprintf(path,"test%d.tga",traceFrameBufferCounter);

      #define IMAGE_WIDTH 640
      #define IMAGE_HEIGHT 400

      uint8_t tga_header[18];
      rb->memset(tga_header, 0, 18);
      tga_header[2] = 2;
      tga_header[12] = (IMAGE_WIDTH & 0x00FF);
      tga_header[13] = (IMAGE_WIDTH  & 0xFF00) / 256;
      tga_header[14] = (IMAGE_HEIGHT  & 0x00FF) ;
      tga_header[15] =(IMAGE_HEIGHT & 0xFF00) / 256;
      tga_header[16] = 32 ;



      // Open the file, write both header and payload, close, done.
      char path[256];
      sprintf(path,"test%d.tga",traceFrameBufferCounter);
      FILE* pScreenshot = fopen(path, "wb");
      fwrite(&tga_header, 18, sizeof(uint8_t), pScreenshot);
      fwrite(allFrameBuffers, IMAGE_WIDTH * IMAGE_HEIGHT,SCREENSHOT_BPP * sizeof(uint8_t),pScreenshot);
      fclose(pScreenshot);
    */


    char path[256];
    //sprintf(path,"%4d%s.png",traceFrameBufferCounter,comment);
    sprintf(path, "%4d.png", traceFrameBufferCounter);

    GL_FCS_SaveAsSpecifiedPNG(path, allFrameBuffers);
}
#endif

#if TRACE_BG_BUFFER



uint8_t bgPalette[48] = {
    0x8, 0x8, 0xC, 0xC, 0xC, 0x15, 0xC, 0x11, 0x1D, 0x15, 0x2A, 0x3F, 0x1D, 0x19, 0x19, 0x37, 0x2E, 0x2A, 0x26, 0x1D, 0x1D, 0x37, 0x26, 0x22, 0x22, 0xC, 0x0, 0x26, 0x33, 0x3F, 0x11, 0x11, 0x15, 0x11, 0x15, 0x1D, 0x15, 0x19, 0x26, 0x15, 0x1D, 0x37, 0x0, 0x26, 0x3F, 0x2E, 0x15, 0x0,
};
void bgWriteLine(uint8_t *dst, uint8_t *src, int size)
{
    uint8_t* dumpPalette;

//      if (!dumpPaletteCursor)
    //  dumpPalette = allPalettesDump[dumpPaletteCursor];
//      else
    dumpPalette = bgPalette;

    for( uint8_t twoPixels = 0 ; twoPixels < size ; twoPixels++)
    {
        int pixelIndex0 = (*src & 0xF0) >> 4;
        pixelIndex0 &= 0x10 - 1;

        int pixelIndex1 = (*src & 0xF);
        pixelIndex1 &= 0x10 - 1;

        //We need to write those two pixels
        dst[0] = dumpPalette[pixelIndex0 * 3] << 2 | dumpPalette[pixelIndex0 * 3];
        dst[1] = dumpPalette[pixelIndex0 * 3 + 1] << 2 | dumpPalette[pixelIndex0 * 3 + 1];
        dst[2] = dumpPalette[pixelIndex0 * 3 + 2] << 2 | dumpPalette[pixelIndex0 * 3 + 2];
        //dst[3] = 0xFF;
        dst += 3;

        dst[0] = dumpPalette[pixelIndex1 * 3] << 2 | dumpPalette[pixelIndex1 * 3];
        dst[1] = dumpPalette[pixelIndex1 * 3 + 1] << 2 | dumpPalette[pixelIndex1 * 3 + 1];
        dst[2] = dumpPalette[pixelIndex1 * 3 + 2] << 2 | dumpPalette[pixelIndex1 * 3 + 2];
        //dst[3] = 0xFF;
        dst += 3;

        src++;
    }
}

void bgDumpFrameBuffer(uint8_t *src, uint8_t *dst, int x, int y)
{

    for (int line = 199 ; line >= 0 ; line--)
    {
        bgWriteLine(dst + x * 3 + y * 320 * 3  , src + line * 160, 160);
        dst += 320 * 3;
    }
}

#include "png.h"
int bgSaveAsSpecifiedPNG(char* path, uint8_t* pixels, int depth = 8, int format = PNG_COLOR_TYPE_RGB)
{
#if 0
    FILE * fp;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_byte ** row_pointers = NULL;
    int status = -1;
    int bytePerPixel = 0;
    int y;

    fp = fopen (path, "wb");
    if (! fp) {
        goto fopen_failed;
    }

    png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        goto png_create_write_struct_failed;
    }

    info_ptr = png_create_info_struct (png_ptr);
    if (info_ptr == NULL) {
        goto png_create_info_struct_failed;
    }

    if (setjmp (png_jmpbuf (png_ptr))) {
        goto png_failure;
    }

    /* Set image attributes. */

    png_set_IHDR (png_ptr,
                  info_ptr,
                  320,
                  200,
                  depth,
                  format,
                  PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_DEFAULT,
                  PNG_FILTER_TYPE_DEFAULT);

    if (format == PNG_COLOR_TYPE_GRAY   )
        bytePerPixel = depth / 8 * 1;
    else
        bytePerPixel = depth / 8 * 3;

    row_pointers = (png_byte **)png_malloc (png_ptr, 200 * sizeof (png_byte *));
    //for (y = vid.height-1; y >=0; --y)
    for (y = 0; y < 200; y++)
    {
        row_pointers[y] = (png_byte*)&pixels[320 * (200 - y) * bytePerPixel];
    }

    png_init_io (png_ptr, fp);
    png_set_rows (png_ptr, info_ptr, row_pointers);
    png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    //png_read_image (png_ptr, info_ptr);//

    status = 0;

    png_free (png_ptr, row_pointers);


png_failure:
png_create_info_struct_failed:
    png_destroy_write_struct (&png_ptr, &info_ptr);

png_create_write_struct_failed:
    fclose (fp);
fopen_failed:
    return status;
#endif
}

int bgFrameBufferCounter = 0;

void video_dumpBackGroundBuffer()
{
    if (v->_curPagePtr1 != v->_pagePtrs[0])
        return;

    uint8_t bgBuffer[320 * 200 * 3];
    bgDumpFrameBuffer(v->_curPagePtr1, bgBuffer, 0, 0);


    char path[256];
    //sprintf(path,"%4d%s.png",traceFrameBufferCounter,comment);
    sprintf(path, "bg%4d.png", bgFrameBufferCounter++);

    bgSaveAsSpecifiedPNG(path, bgBuffer);
}

#endif
