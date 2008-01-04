/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* New greyscale framework
* Scrolling routines
*
* This is a generic framework to display 129 shades of grey on low-depth
* bitmap LCDs (Archos b&w, Iriver & Ipod 4-grey) within plugins.
*
* Copyright (C) 2008 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "plugin.h"
#include "grey.h"

/*** Scrolling ***/

/* Scroll left */
void grey_scroll_left(int count)
{
    unsigned char *data, *data_end;
    int length, blank;

    if ((unsigned)count >= (unsigned)_grey_info.width)
        return;

    data = _grey_info.buffer;
    data_end = data + _GREY_MULUQ(_grey_info.width, _grey_info.height);
    length = _grey_info.width - count;
    blank = (_grey_info.drawmode & DRMODE_INVERSEVID) ?
             _grey_info.fg_val : _grey_info.bg_val;

    do
    {
        _grey_rb->memmove(data, data + count, length);
        _grey_rb->memset(data + length, blank, count);
        data += _grey_info.width;
    }
    while (data < data_end);
}

/* Scroll right */
void grey_scroll_right(int count)
{
    unsigned char *data, *data_end;
    int length, blank;

    if ((unsigned)count >= (unsigned)_grey_info.width)
        return;

    data = _grey_info.buffer;
    data_end = data + _GREY_MULUQ(_grey_info.width, _grey_info.height);
    length = _grey_info.width - count;
    blank = (_grey_info.drawmode & DRMODE_INVERSEVID) ?
             _grey_info.fg_val : _grey_info.bg_val;

    do
    {
        _grey_rb->memmove(data + count, data, length);
        _grey_rb->memset(data, blank, count);
        data += _grey_info.width;
    }
    while (data < data_end);
}

/* Scroll up */
void grey_scroll_up(int count)
{
    long shift, length;
    int blank;

    if ((unsigned)count >= (unsigned)_grey_info.height)
        return;

    shift = _GREY_MULUQ(_grey_info.width, count);
    length = _GREY_MULUQ(_grey_info.width, _grey_info.height - count);
    blank = (_grey_info.drawmode & DRMODE_INVERSEVID) ?
             _grey_info.fg_val : _grey_info.bg_val;

    _grey_rb->memmove(_grey_info.buffer, _grey_info.buffer + shift, length);
    _grey_rb->memset(_grey_info.buffer + length, blank, shift);
}

/* Scroll down */
void grey_scroll_down(int count)
{
    long shift, length;
    int blank;

    if ((unsigned)count >= (unsigned)_grey_info.height)
        return;

    shift = _GREY_MULUQ(_grey_info.width, count);
    length = _GREY_MULUQ(_grey_info.width, _grey_info.height - count);
    blank = (_grey_info.drawmode & DRMODE_INVERSEVID) ?
             _grey_info.fg_val : _grey_info.bg_val;

    _grey_rb->memmove(_grey_info.buffer + shift, _grey_info.buffer, length);
    _grey_rb->memset(_grey_info.buffer, blank, shift);
}

/*** Unbuffered scrolling functions ***/

#ifdef SIMULATOR

/* Scroll left */
void grey_ub_scroll_left(int count)
{
    grey_scroll_left(count);
    grey_update();
}

/* Scroll right */
void grey_ub_scroll_right(int count)
{
    grey_scroll_right(count);
    grey_update();
}

/* Scroll up */
void grey_ub_scroll_up(int count)
{
    grey_scroll_up(count);
    grey_update();
}

/* Scroll down */
void grey_ub_scroll_down(int count)
{
    grey_scroll_down(count);
    grey_update();
}

#else /* !SIMULATOR */

/* Scroll left */
void grey_ub_scroll_left(int count)
{
    unsigned char *dst, *src, *end;
    int blank, y, idx;

    if ((count == 0) || ((unsigned)count >= (unsigned)_grey_info.width))
        return;
        
    blank = (_grey_info.drawmode & DRMODE_INVERSEVID) ?
             _grey_info.fg_val : _grey_info.bg_val;

    for (y = 0; y < _grey_info.height; y++)
    {
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
        idx = _GREY_MULUQ(_grey_info.width, y);
#else
#if LCD_DEPTH == 1
        idx = _GREY_MULUQ(_grey_info.width, y & ~7) + (~y & 7);
#elif LCD_DEPTH == 2
        idx = _GREY_MULUQ(_grey_info.width, y & ~3) + (~y & 3);
#endif
#endif /* LCD_PIXELFORMAT */
        dst = &_grey_info.data[idx].value;
        src = dst + count * _GREY_X_ADVANCE;
        end = dst + _grey_info.width * _GREY_X_ADVANCE;
        
        do
        {
            *dst = *src;
            dst += _GREY_X_ADVANCE;
            src += _GREY_X_ADVANCE;
        }
        while (src < end);
        
        do
        {
            *dst = blank;
            dst += _GREY_X_ADVANCE;
        }
        while (dst < end);
    }
}

/* Scroll right */
void grey_ub_scroll_right(int count)
{
    unsigned char *dst, *src, *start;
    int blank, y, idx;

    if ((count == 0) || ((unsigned)count >= (unsigned)_grey_info.width))
        return;
        
    blank = (_grey_info.drawmode & DRMODE_INVERSEVID) ?
             _grey_info.fg_val : _grey_info.bg_val;

    for (y = 0; y < _grey_info.height; y++)
    {
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
        idx = _GREY_MULUQ(_grey_info.width, y);
#else
#if LCD_DEPTH == 1
        idx = _GREY_MULUQ(_grey_info.width, y & ~7) + (~y & 7);
#elif LCD_DEPTH == 2
        idx = _GREY_MULUQ(_grey_info.width, y & ~3) + (~y & 3);
#endif
#endif /* LCD_PIXELFORMAT */
        start = &_grey_info.data[idx].value;
        dst = start + _grey_info.width * _GREY_X_ADVANCE;
        src = dst - count * _GREY_X_ADVANCE;
        
        do 
        {
            dst -= _GREY_X_ADVANCE;
            src -= _GREY_X_ADVANCE;
            *dst = *src;
        }
        while (src > start);
        
        do
        {
            dst -= _GREY_X_ADVANCE;
            *dst = blank;
        }
        while (dst > start);
    }
}

void grey_ub_scroll_up(int count)
{
    unsigned char *dst, *dst_end, *src;
    int blank, ys, yd, is, id;

    if ((unsigned)count >= (unsigned)_grey_info.height)
        return;
        
    blank = (_grey_info.drawmode & DRMODE_INVERSEVID) ?
             _grey_info.fg_val : _grey_info.bg_val;

    for (ys = count, yd = 0; ys < _grey_info.height; ys++, yd++)
    {
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
        id = _GREY_MULUQ(_grey_info.width, yd);
        is = _GREY_MULUQ(_grey_info.width, ys);
#else
#if LCD_DEPTH == 1
        id = _GREY_MULUQ(_grey_info.width, yd & ~7) + (~yd & 7);
        is = _GREY_MULUQ(_grey_info.width, ys & ~7) + (~ys & 7);
#elif LCD_DEPTH == 2
        id = _GREY_MULUQ(_grey_info.width, yd & ~3) + (~yd & 3);
        is = _GREY_MULUQ(_grey_info.width, ys & ~3) + (~ys & 3);
#endif
#endif /* LCD_PIXELFORMAT */
        dst = &_grey_info.data[id].value;
        src = &_grey_info.data[is].value;
        dst_end = dst + _grey_info.width * _GREY_X_ADVANCE;

        do
        {
            *dst = *src;
            dst += _GREY_X_ADVANCE;
            src += _GREY_X_ADVANCE;
        }
        while (dst < dst_end);
    }
    for (; yd < _grey_info.height; yd++)
    {
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
        id = _GREY_MULUQ(_grey_info.width, yd);
#else
#if LCD_DEPTH == 1
        id = _GREY_MULUQ(_grey_info.width, yd & ~7) + (~yd & 7);
#elif LCD_DEPTH == 2
        id = _GREY_MULUQ(_grey_info.width, yd & ~3) + (~yd & 3);
#endif
#endif /* LCD_PIXELFORMAT */
        dst = &_grey_info.data[id].value;
        dst_end = dst + _grey_info.width * _GREY_X_ADVANCE;

        do
        {
            *dst = blank;
            dst += _GREY_X_ADVANCE;
        }
        while (dst < dst_end);
    }
}

void grey_ub_scroll_down(int count)
{
    unsigned char *dst, *dst_end, *src;
    int blank, ys, yd, is, id;

    if ((unsigned)count >= (unsigned)_grey_info.height)
        return;
        
    blank = (_grey_info.drawmode & DRMODE_INVERSEVID) ?
             _grey_info.fg_val : _grey_info.bg_val;

    yd = _grey_info.height - 1;
    for (ys = yd - count; ys >= 0; ys--, yd--)
    {
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
        id = _GREY_MULUQ(_grey_info.width, yd);
        is = _GREY_MULUQ(_grey_info.width, ys);
#else
#if LCD_DEPTH == 1
        id = _GREY_MULUQ(_grey_info.width, yd & ~7) + (~yd & 7);
        is = _GREY_MULUQ(_grey_info.width, ys & ~7) + (~ys & 7);
#elif LCD_DEPTH == 2
        id = _GREY_MULUQ(_grey_info.width, yd & ~3) + (~yd & 3);
        is = _GREY_MULUQ(_grey_info.width, ys & ~3) + (~ys & 3);
#endif
#endif /* LCD_PIXELFORMAT */
        dst = &_grey_info.data[id].value;
        src = &_grey_info.data[is].value;
        dst_end = dst + _grey_info.width * _GREY_X_ADVANCE;

        do
        {
            *dst = *src;
            dst += _GREY_X_ADVANCE;
            src += _GREY_X_ADVANCE;
        }
        while (dst < dst_end);
    }
    for (; yd >= 0; yd--)
    {
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
        id = _GREY_MULUQ(_grey_info.width, yd);
#else
#if LCD_DEPTH == 1
        id = _GREY_MULUQ(_grey_info.width, yd & ~7) + (~yd & 7);
#elif LCD_DEPTH == 2
        id = _GREY_MULUQ(_grey_info.width, yd & ~3) + (~yd & 3);
#endif
#endif /* LCD_PIXELFORMAT */
        dst = &_grey_info.data[id].value;
        dst_end = dst + _grey_info.width * _GREY_X_ADVANCE;

        do
        {
            *dst = blank;
            dst += _GREY_X_ADVANCE;
        }
        while (dst < dst_end);
    }
}

#endif /* !SIMULATOR */
