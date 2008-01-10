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
        data += length;
        _grey_rb->memset(data, blank, count);
        data += count;
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
    unsigned char *data, *data_end;
    int blank, length;

    if ((unsigned)count >= (unsigned)_grey_info.width)
        return;
    
    data = _grey_info.values;
    data_end = data + _GREY_MULUQ(_grey_info.width, _grey_info.height);
    length = (_grey_info.width - count) << _GREY_BSHIFT;
    count <<= _GREY_BSHIFT;
    blank = (_grey_info.drawmode & DRMODE_INVERSEVID) ?
             _grey_info.fg_val : _grey_info.bg_val;

    do
    {
        _grey_rb->memmove(data, data + count, length);
        data += length;
        _grey_rb->memset(data, blank, count);
        data += count;
    }
    while (data < data_end);
}

/* Scroll right */
void grey_ub_scroll_right(int count)
{
    unsigned char *data, *data_end;
    int blank, length;

    if ((unsigned)count >= (unsigned)_grey_info.width)
        return;
    
    data = _grey_info.values;
    data_end = data + _GREY_MULUQ(_grey_info.width, _grey_info.height);
    length = (_grey_info.width - count) << _GREY_BSHIFT;
    count <<= _GREY_BSHIFT;
    blank = (_grey_info.drawmode & DRMODE_INVERSEVID) ?
             _grey_info.fg_val : _grey_info.bg_val;

    do
    {
        _grey_rb->memmove(data + count, data, length);
        _grey_rb->memset(data, blank, count);
        data += _grey_info.width << _GREY_BSHIFT;
    }
    while (data < data_end);
}

/* Scroll up */
void grey_ub_scroll_up(int count)
{
    unsigned char *dst, *end, *src;
    int blank;

    if ((unsigned)count >= (unsigned)_grey_info.height)
        return;
        
    dst   = _grey_info.values;
    end   = dst + _GREY_MULUQ(_grey_info.height, _grey_info.width);
    blank = (_grey_info.drawmode & DRMODE_INVERSEVID) ?
             _grey_info.fg_val : _grey_info.bg_val;

#if LCD_PIXELFORMAT == VERTICAL_PACKING
    if (count & _GREY_BMASK)
    {
        /* Scrolling by fractional blocks - move pixel wise. */
        unsigned char *line_end;
        int ys, yd;

        for (ys = count, yd = 0; ys < _grey_info.height; ys++, yd++)
        {
            dst = _grey_info.values
                + _GREY_MULUQ(_grey_info.width, yd & ~_GREY_BMASK)
                + (~yd & _GREY_BMASK);
            src = _grey_info.values
                + _GREY_MULUQ(_grey_info.width, ys & ~_GREY_BMASK)
                + (~ys & _GREY_BMASK);
            line_end = dst + _grey_info.width * _GREY_BSIZE;

            do
            {
                *dst = *src;
                dst += _GREY_BSIZE;
                src += _GREY_BSIZE;
            }
            while (dst < line_end);
        }
        for (; yd & _GREY_BMASK; yd++)   /* Fill remainder of current block. */
        {
            dst = _grey_info.values
                + _GREY_MULUQ(_grey_info.width, yd & ~_GREY_BMASK)
                + (~yd & _GREY_BMASK);
            line_end = dst + _grey_info.width * _GREY_BSIZE;

            do
            {
                *dst = blank;
                dst += _GREY_BSIZE;
            }
            while (dst < line_end);
        }
    }
    else
#endif
    {
        int blen = _GREY_MULUQ(_grey_info.height - count, _grey_info.width);

        src = dst + _GREY_MULUQ(count, _grey_info.width);
        _grey_rb->memmove(dst, src, blen);
        dst += blen;
    }
    _grey_rb->memset(dst, blank, end - dst); /* Fill remainder at once. */
}

/* Scroll down */
void grey_ub_scroll_down(int count)
{
    unsigned char *start, *dst, *src;
    int blank;

    if ((unsigned)count >= (unsigned)_grey_info.height)
        return;
        
    start = _grey_info.values;
    dst   = start + _GREY_MULUQ(_grey_info.height, _grey_info.width);
    blank = (_grey_info.drawmode & DRMODE_INVERSEVID) ?
             _grey_info.fg_val : _grey_info.bg_val;

#if LCD_PIXELFORMAT == VERTICAL_PACKING
    if (count & _GREY_BMASK)
    {
        /* Scrolling by fractional blocks - move pixel wise. */
        unsigned char *line_end;
        int ys, yd;

        yd = _grey_info.height - 1;
        for (ys = yd - count; ys >= 0; ys--, yd--)
        {
            dst = _grey_info.values
                + _GREY_MULUQ(_grey_info.width, yd & ~_GREY_BMASK)
                + (~yd & _GREY_BMASK);
            src = _grey_info.values
                + _GREY_MULUQ(_grey_info.width, ys & ~_GREY_BMASK)
                + (~ys & _GREY_BMASK);
            line_end = dst + _grey_info.width * _GREY_BSIZE;

            do
            {
                *dst = *src;
                dst += _GREY_BSIZE;
                src += _GREY_BSIZE;
            }
            while (dst < line_end);
        }
        for (; ~yd & _GREY_BMASK; yd--)   /* Fill remainder of current block. */
        {
            dst = _grey_info.values
                + _GREY_MULUQ(_grey_info.width, yd & ~_GREY_BMASK)
                + (~yd & _GREY_BMASK);
            line_end = dst + _grey_info.width * _GREY_BSIZE;

            do
            {
                line_end -= _GREY_BSIZE;
                *line_end = blank;
            }
            while (dst < line_end);
        }
    }
    else
#endif
    {
        int blen = _GREY_MULUQ(_grey_info.height - count, _grey_info.width);

        dst -= blen;
        _grey_rb->memmove(dst, start, blen);
    }
    _grey_rb->memset(start, blank, dst - start); /* Fill remainder at once. */
}

#endif /* !SIMULATOR */
