/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (c) 2012 Marcin Bukat
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

#include <lib/pluginlib_bmp.h>
#include "bmp.h"
#if LCD_DEPTH < 8
#include <lib/grey.h>
#endif

#include "gif_lib.h"
#include "gif_decoder.h"

#ifndef resize_bitmap
#if defined(HAVE_LCD_COLOR)
#define resize_bitmap   smooth_resize_bitmap
#else
#define resize_bitmap   grey_resize_bitmap
#endif
#endif

#if defined(HAVE_LCD_COLOR)
typedef struct uint8_rgb pixel_t;
#define NATIVE_SZ (GifFile->SWidth*GifFile->SHeight*FB_DATA_SZ)
#define PIXEL_TRANSPARENT 0x00
#else
typedef unsigned char pixel_t;
#define NATIVE_SZ (GifFile->SWidth*GifFile->SHeight)
#define PIXEL_TRANSPARENT 0xff
#endif

#define PIXELS_SZ (GifFile->SWidth*GifFile->SHeight*sizeof(pixel_t))

static GifFileType *GifFile;

static void gif2pixels(GifPixelType *Line, pixel_t *out,
                       int Row, int Col, int Width)
{
    int x;
#ifndef HAVE_LCD_COLOR
    struct uint8_rgb rgb;
#endif

    GifColorType *ColorMapEntry;

    /* Color map to use */
    ColorMapObject *ColorMap = (GifFile->Image.ColorMap ?
                                GifFile->Image.ColorMap :
                                GifFile->SColorMap);

    pixel_t *pixel = out + ((Row * GifFile->SWidth) + Col);

    for (x = 0; x < Width; x++, pixel++)
    {
        ColorMapEntry = &ColorMap->Colors[Line[x]];

        if (GifFile->Image.GCB &&
            GifFile->Image.GCB->TransparentColor == Line[x])
            continue;

#ifdef HAVE_LCD_COLOR
        pixel->red = ColorMapEntry->Red;
        pixel->green = ColorMapEntry->Green;
        pixel->blue = ColorMapEntry->Blue;
#else
        rgb.red = ColorMapEntry->Red;
        rgb.green = ColorMapEntry->Green;
        rgb.blue = ColorMapEntry->Blue;

        *pixel = brightness(rgb);
#endif
    }
}

static void pixels2native(struct scaler_context *ctx,
                          pixel_t *pixels_buffer,
                          int Row)
{
#ifdef HAVE_LCD_COLOR
    const struct custom_format *cformat = &format_native;
#else
    const struct custom_format *cformat = &format_grey;
#endif

    void (*output_row_8)(uint32_t, void*, struct scaler_context*) =
        cformat->output_row_8;

    output_row_8(Row, (void *)(pixels_buffer + (Row*ctx->bm->width)), ctx);
}

void gif_decoder_init(struct gif_decoder *d, void *mem, size_t size)
{
    memset(d, 0, sizeof(struct gif_decoder));

    d->mem = mem;
    d->mem_size = size;

    /* mem allocator init */
    init_memory_pool(d->mem_size, d->mem);
}

void gif_decoder_destroy_memory_pool(struct gif_decoder *d)
{
    destroy_memory_pool(d->mem);
}

void gif_open(char *filename, struct gif_decoder *d)
{
    if ((GifFile = DGifOpenFileName(filename, &d->error)) == NULL)
        return;

    d->width = GifFile->SWidth;
    d->height = GifFile->SHeight;
    d->frames_count = 0;
}

static void set_canvas_background(pixel_t *out, GifFileType *GifFile)
{
    /* Reading Gif spec it seems one should always use background color
     * in canvas but most real files omit this and sets background color to 0
     * (which IS valid index). We can choose to either conform to standard
     * (and wrongly display most of gifs with transparency) or stick to
     * common practise and treat background color 0 as transparent.
     * Moreover when dispose method is BACKGROUND spec suggest
     * to reset canvas to global background color specified in gif BUT
     * all renderers I know use transparency instead.
     */
    memset(out, PIXEL_TRANSPARENT, PIXELS_SZ);
}

/* var names adhere to giflib coding style */
void gif_decode(struct gif_decoder *d,
                void (*pf_progress)(int current, int total))
{
    int i, j;

    int Size;
    int Row;
    int Col;
    int Width;
    int Height;
    int ExtCode;

    GifPixelType *Line;

    GifRecordType RecordType;
    GifByteType *Extension;

    unsigned char *out = NULL;

    /* The way Interlaced image should
     * be read - offsets and jumps
     */
    const char InterlacedOffset[] = { 0, 4, 2, 1 };
    const char InterlacedJumps[] = { 8, 8, 4, 2 };

    /* used for color conversion */
    struct bitmap bm;
    struct scaler_context ctx = {
        .bm = &bm,
        .dither = 0
    };

    /* initialize struct */
    memset(&bm, 0, sizeof(struct bitmap));

    Size = GifFile->SWidth * sizeof(GifPixelType); /* Size in bytes one row.*/
    Line = (GifPixelType *)malloc(Size);
    if (Line == NULL)
    {
        /* error allocating temp space */
        d->error = D_GIF_ERR_NOT_ENOUGH_MEM;
        DGifCloseFile(GifFile);
        return;
    }

    /* We use two pixel buffers if dispose method asks
     * for restoration of the previous state.
     * We only swap the indexes leaving data in place.
     */
    int buf_idx = 0;
    pixel_t *pixels_buffer[2];
    pixels_buffer[0] = (pixel_t *)malloc(PIXELS_SZ);
    pixels_buffer[1] = NULL;

    if (pixels_buffer[0] == NULL)
    {
        d->error = D_GIF_ERR_NOT_ENOUGH_MEM;
        goto free_and_return;
    }

    /* Global background color */
    set_canvas_background(pixels_buffer[0], GifFile);

    bm.width = GifFile->SWidth;
    bm.height = GifFile->SHeight;
    d->native_img_size = NATIVE_SZ;

    if (pf_progress != NULL)
        pf_progress(0, 100);

    /* Scan the content of the GIF file and load the image(s) in: */
    do
    {
        if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR)
        {
            d->error = GifFile->Error;
            goto free_and_return;
        }

        switch (RecordType)
        {
            case IMAGE_DESC_RECORD_TYPE:

                if (DGifGetImageDesc(GifFile) == GIF_ERROR)
                {
                    d->error = GifFile->Error;
                    goto free_and_return;
                }

                /* Image Position relative to canvas */
                Row = GifFile->Image.Top;
                Col = GifFile->Image.Left;
                Width = GifFile->Image.Width;
                Height = GifFile->Image.Height;

                /* Check Color map to use */
                if (GifFile->Image.ColorMap == NULL &&
                    GifFile->SColorMap == NULL)
                {
                    d->error = D_GIF_ERR_NO_COLOR_MAP;
                    goto free_and_return;
                }

                /* sanity check */
                if (GifFile->Image.Left+GifFile->Image.Width>GifFile->SWidth ||
                    GifFile->Image.Top+GifFile->Image.Height>GifFile->SHeight)
                {
                    d->error = D_GIF_ERR_DATA_TOO_BIG;
                    goto free_and_return;
                }

                if (GifFile->Image.GCB &&
                    GifFile->Image.GCB->DisposalMode == DISPOSE_PREVIOUS)
                {
                    /* We need to take a snapshot before processing the image
                     * in order to restore canvas to previous state after
                     * rendering
                     */
                     buf_idx ^= 1;

                     if (pixels_buffer[buf_idx] == NULL)
                         pixels_buffer[buf_idx] = (pixel_t *)malloc(PIXELS_SZ);
                }

                if (GifFile->Image.Interlace)
                {
                    /* Need to perform 4 passes on the image */
                    for (i = 0; i < 4; i++)
                    {
                        for (j = Row + InterlacedOffset[i];
                             j < Row + Height;
                             j += InterlacedJumps[i])
                        {
                            if (DGifGetLine(GifFile, Line, Width) == GIF_ERROR)
                            {
                                d->error = GifFile->Error;
                                goto free_and_return;
                            }

                            gif2pixels(Line, pixels_buffer[buf_idx],
                                       Row + j, Col, Width);
                        }

                        if (pf_progress != NULL)
                            pf_progress(25*(i+1), 100);
                    }
                }
                else
                {
                    for (i = 0; i < Height; i++)
                    {
                        /* load single line into buffer */
                        if (DGifGetLine(GifFile,  Line, Width) == GIF_ERROR)
                        {
                            d->error = GifFile->Error;
                            goto free_and_return;
                        }

                        gif2pixels(Line, pixels_buffer[buf_idx],
                                   Row + i, Col, Width);

                        if (pf_progress != NULL)
                            pf_progress((i+1), Height);
                    }
                }

                /* allocate space for new frame */
                out = realloc(out, d->native_img_size*(d->frames_count + 1));
                if (out == NULL)
                {
                    d->error = D_GIF_ERR_NOT_ENOUGH_MEM;
                    goto free_and_return;
                }

                bm.data = out + d->native_img_size*d->frames_count;

                /* animated gif */
                if (GifFile->Image.GCB && GifFile->Image.GCB->DelayTime != 0)
                {
                    for (i=0; i < ctx.bm->height; i++)
                        pixels2native(&ctx, (void *)pixels_buffer[buf_idx], i);

                    /* restore to the background color */
                    switch (GifFile->Image.GCB->DisposalMode)
                    {
                        case DISPOSE_BACKGROUND:
                            set_canvas_background(pixels_buffer[buf_idx],
                                                  GifFile);
                            break;

                        case DISPOSE_PREVIOUS:
                            buf_idx ^= 1;
                            break;

                        default:
                            /* DISPOSAL_UNSPECIFIED
                             * DISPOSE_DO_NOT
                             */
                            break;
                    }

                    d->frames_count++;
                }

                break;

            case EXTENSION_RECORD_TYPE:
                if (DGifGetExtension(GifFile, &ExtCode, &Extension) ==
                    GIF_ERROR)
                {
                    d->error = GifFile->Error;
                    goto free_and_return;
                }

                if (ExtCode == GRAPHICS_EXT_FUNC_CODE)
                {
                    if (GifFile->Image.GCB == NULL)
                        GifFile->Image.GCB = (GraphicsControlBlock *)
                            malloc(sizeof(GraphicsControlBlock));

                    if (DGifExtensionToGCB(Extension[0],
                                           Extension + 1,
                                           GifFile->Image.GCB) == GIF_ERROR)
                    {
                        d->error = GifFile->Error;
                        goto free_and_return;
                    }
                    d->delay = GifFile->Image.GCB->DelayTime;
                }

                /* Skip anything else */
                while (Extension != NULL)
                {
                    if (DGifGetExtensionNext(GifFile, &Extension) == GIF_ERROR)
                    {
                        d->error = GifFile->Error;
                        goto free_and_return;
                    }
                }
                break;

            /* including TERMINATE_RECORD_TYPE */
            default:
                break;
        }

    } while (RecordType != TERMINATE_RECORD_TYPE);

    /* free all internal allocated data */
    if (DGifCloseFile(GifFile) == GIF_ERROR)
    {
        d->error = GifFile->Error;
        free(pixels_buffer[0]);
        if (pixels_buffer[1])
            free(pixels_buffer[1]);
        free(Line);
        return;
    }

    /* not animated gif */
    if (d->frames_count == 0)
    {
        for (i=0; i < ctx.bm->height; i++)
            pixels2native(&ctx, (void *)pixels_buffer[buf_idx], i);

        d->frames_count++;
    }

    free(pixels_buffer[0]);
    if (pixels_buffer[1])
        free(pixels_buffer[1]);

    free(Line);

    /* WARNING !!!! */
    /* GifFile object is trashed from now on, DONT use it */
    /* Move bitmap in native format to the front of the buff */
    memmove(d->mem, out, d->frames_count*d->native_img_size);

    /* correct aspect ratio */
#if (LCD_PIXEL_ASPECT_HEIGHT != 1 || LCD_PIXEL_ASPECT_WIDTH != 1)
    struct bitmap img_src, img_dst;    /* scaler vars */
    struct dim dim_src, dim_dst;       /* recalc_dimensions vars */
    size_t c_native_img_size;          /* size of the image after correction */

    dim_src.width = bm.width;
    dim_src.height = bm.height;

    dim_dst.width = bm.width;
    dim_dst.height = bm.height;

    /* defined in apps/recorder/resize.c */
    if (!recalc_dimension(&dim_dst, &dim_src))
    {
        /* calculate 'corrected' image size */
#ifdef HAVE_LCD_COLOR
        c_native_img_size = dim_dst.width * dim_dst.height * FB_DATA_SZ;
#else
        c_native_img_size = dim_dst.width * dim_dst.height;
#endif

        /* check memory constraints
         * do the correction only if there is enough
         * free memory
         */
        if (d->native_img_size*d->frames_count + c_native_img_size <=
            d->mem_size)
        {
            img_dst.width = dim_dst.width;
            img_dst.height = dim_dst.height;
            img_dst.data = (unsigned char *)d->mem +
                                            d->native_img_size*d->frames_count;

            for (i = 0; i < d->frames_count; i++)
            {
                img_src.width = dim_src.width;
                img_src.height = dim_src.height;
                img_src.data = (unsigned char *)d->mem + i*d->native_img_size;

                /* scale the bitmap to correct physical
                 * pixel dimentions
                 */
                resize_bitmap(&img_src, &img_dst);

                /* copy back corrected image */
                memmove(d->mem + i*c_native_img_size,
                        img_dst.data,
                        c_native_img_size);
            }

            /* update decoder struct */
            d->width = img_dst.width;
            d->height = img_dst.height;
            d->native_img_size = c_native_img_size;
        }
    }
#endif
    return;

free_and_return:
    if (Line)
        free(Line);
    if (pixels_buffer[0])
        free(pixels_buffer[0]);
    if (pixels_buffer[1])
        free(pixels_buffer[1]);
    DGifCloseFile(GifFile);
    return;
}
