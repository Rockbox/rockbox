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

static GifFileType *GifFile;

#if 0
typedef struct {
    unsigned int byteCount;
    char         *bytes;
} ExtBlock;

static int ext_block_append(ExtBlock *extBlock,
                            unsigned int len,
                            unsigned char extData[])
{
    extBlock->bytes = realloc(extBlock->bytes,extBlock->byteCount+len);
    if (extBlock->bytes == NULL)
    {
        return (GIF_ERROR);
    }

    memcpy(&(extBlock->bytes[extBlock->byteCount]), &extData[0], len);
    extBlock->byteCount += len;
    return (GIF_OK);
}
#endif

/* convert one decoded gif line into 24bit RGB
 * and then to native pixel format using
 * dedicated function
 */
static void gif2native(struct scaler_context *ctx,
                       ColorMapObject *ColorMap,
                       GraphicsControlBlock *GCB,
                       GifPixelType *Line,
                       int Row)
{
    int x;
    GifColorType *ColorMapEntry;

#ifdef HAVE_LCD_COLOR
    struct uint8_rgb *pixel;
    struct uint8_rgb *buf = (struct uint8_rgb *)malloc(ctx->bm->width * sizeof(struct uint8_rgb));
    const struct custom_format *cformat = &format_native;
#else
    unsigned char *pixel;
    uint8_t *buf = (unsigned char *)malloc(ctx->bm->width);
    struct uint8_rgb rgb;
    const struct custom_format *cformat = &format_grey;
#endif

    void (*output_row_8)(uint32_t, void*, struct scaler_context*) = cformat->output_row_8;
    pixel = buf;

    for (x = 0; x < ctx->bm->width; x++)
    {
        ColorMapEntry = &ColorMap->Colors[Line[x]];

#ifdef HAVE_LCD_COLOR
        /* Assumes black background */
        /* set RGB tripple according to gif colormap */
        if (GCB && GCB->TransparentColor == Line[x])
        {
            pixel->red = 0;
            pixel->green = 0;
            pixel->blue = 0;
        }
        else
        {
            pixel->red = ColorMapEntry->Red;
            pixel->green = ColorMapEntry->Green;
            pixel->blue = ColorMapEntry->Blue;
        }
#else
        /* set RGB tripple according to gif colormap
         * and calculate luminance value
         */
        if (GCB && GCB->TransparentColor == Line[x])
            *pixel = 255;
        else
        {
            rgb.red = ColorMapEntry->Red;
            rgb.green = ColorMapEntry->Green;
            rgb.blue = ColorMapEntry->Blue;
            *pixel = brightness(rgb);
        }
#endif
        pixel++;
    }

    output_row_8(Row, (void *)buf, ctx);

    free(buf);
}

void gif_decoder_init(struct gif_decoder *d, void *mem, size_t size)
{
    memset(d, 0, sizeof(struct gif_decoder));

    d->mem = mem;
    d->mem_size = size;

    /* mem allocator init */
    init_memory_pool(d->mem_size, d->mem);
}

void gif_open(char *filename, struct gif_decoder *d)
{
    if ((GifFile = DGifOpenFileName(filename, &d->error)) == NULL)
        return;

    d->width = GifFile->SWidth;
    d->height = GifFile->SHeight;
}

/* var names adhere to giflib coding style */
void gif_decode(struct gif_decoder *d,
                void (*pf_progress)(int current, int total))
{
    bool skip = false;
    int i, j;

    int Size;
    int Row;
    int Width;
    int Height;
    int ExtCode;

    GifPixelType *Line;
    GifRecordType RecordType;
    ColorMapObject *ColorMap;
    GraphicsControlBlock *GCB = NULL;
    GifByteType *Extension;

    const char InterlacedOffset[] = { 0, 4, 2, 1 }; /* The way Interlaced image should. */
    const char InterlacedJumps[] = { 8, 8, 4, 2 };  /* be read - offsets and jumps... */

    /* used for color conversion */
    struct bitmap bm;
    struct scaler_context ctx = {
        .bm = &bm,
        .dither = 0
    };

    Size = GifFile->SWidth * sizeof(GifPixelType); /* Size in bytes one row.*/
    Line = (GifPixelType *)malloc(Size); /* allocate space for one row */

    bm.width = GifFile->SWidth;
    bm.height = GifFile->SHeight;
#ifdef HAVE_LCD_COLOR
    d->native_img_size = GifFile->SWidth * GifFile->SHeight * FB_DATA_SZ;
    bm.data = malloc(d->native_img_size);
#else
    d->native_img_size = GifFile->SWidth * GifFile->SHeight;
    bm.data = malloc(d->native_img_size);
#endif

    if (Line == NULL)
    {
        // error allocating temp space
        d->error = D_GIF_ERR_NOT_ENOUGH_MEM;
        return;
    }

    if (pf_progress != NULL)
        pf_progress(0, 100);

    /* Scan the content of the GIF file and load the image(s) in: */
    do
    {
        if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR)
        {
            d->error = GifFile->Error;
            return;
        }

        switch (RecordType)
        {
            case IMAGE_DESC_RECORD_TYPE:

                if (DGifGetImageDesc(GifFile) == GIF_ERROR)
                {
                    d->error = GifFile->Error;
                    return;
                }

                /* Image Position relative to canvas */
                Row = GifFile->Image.Top;
                Width = GifFile->Image.Width;
                Height = GifFile->Image.Height;

                /* Color map to use */
                ColorMap = (GifFile->Image.ColorMap ?
                            GifFile->Image.ColorMap :
                            GifFile->SColorMap);

                if (ColorMap == NULL)
                {
                    d->error = D_GIF_ERR_NO_COLOR_MAP;
                    return;
                }

                /* sanity check */
                if (GifFile->Image.Left + GifFile->Image.Width > GifFile->SWidth ||
                   GifFile->Image.Top + GifFile->Image.Height > GifFile->SHeight)
                {
                    d->error = D_GIF_ERR_DATA_TOO_BIG;
                    return;
                }

                if (GifFile->Image.Interlace)
                {
                    /* Need to perform 4 passes on the image */
                    for (i = 0; i < 4; i++)
                    {
                        for (j = Row + InterlacedOffset[i]; j < Row + Height; j += InterlacedJumps[i])
                        {
                            if (DGifGetLine(GifFile, Line, Width) == GIF_ERROR)
                            {
                                d->error = GifFile->Error;
                                return;
                            }

                            if (!skip || (GCB && GCB->DelayTime == 0))
                                gif2native(&ctx, ColorMap, GCB, Line, j);
                        }

                        pf_progress(25*(i+1), 100);
                    }
                }
                else
                {
                    for (i = 0; i < Height; i++)
                    {
                        /* load single line into buffer */
                        if (DGifGetLine(GifFile, Line, Width) == GIF_ERROR)
                        {
                            d->error = GifFile->Error;
                            return;
                        }

                        if (!skip || (GCB && GCB->DelayTime == 0))
                            gif2native(&ctx, ColorMap, GCB, Line, i);

                        pf_progress(100*(i+1)/Height, 100);
                    }
                }
                skip = true;

                break;

            case EXTENSION_RECORD_TYPE:
                if (DGifGetExtension(GifFile, &ExtCode, &Extension) == GIF_ERROR)
                {
                    d->error = GifFile->Error;
                    return;
                }

                if (ExtCode == GRAPHICS_EXT_FUNC_CODE)
                {
                    GCB = (GraphicsControlBlock *)malloc(sizeof(GraphicsControlBlock));

                    if (DGifExtensionToGCB(Extension[0], Extension+1, GCB) == GIF_ERROR)
                    {
                        d->error = GifFile->Error;
                        return;
                    }
                }

                /* Skip anything else */
                while (Extension != NULL)
                {
                    if (DGifGetExtensionNext(GifFile, &Extension) == GIF_ERROR)
                    {
                        d->error = GifFile->Error;
                        return;
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
        return;
    }
 
    free(GCB);

    /* WARNING !!!! */
    /* GifFile object is trashed from now on, DONT use it */
    /* Move bitmap in native format to the front of the buff */
    memmove(d->mem, bm.data, d->native_img_size);

    /* correct aspect ratio */
#if (LCD_PIXEL_ASPECT_HEIGHT != 1 || LCD_PIXEL_ASPECT_WIDTH != 1)
    struct bitmap img_src, img_dst;    /* scaler vars */
    struct dim dim_src, dim_dst;       /* recalc_dimensions vars */
    size_t c_native_img_size;             /* size of the image after correction */

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
        if (d->native_img_size + c_native_img_size <= d->mem_size)
        {
            img_src.width = dim_src.width;
            img_src.height = dim_src.height;
            img_src.data = (unsigned char *)d->mem;

            img_dst.width = dim_dst.width;
            img_dst.height = dim_dst.height;
            img_dst.data = (unsigned char *)d->mem + d->native_img_size;

            /* scale the bitmap to correct physical
             * pixel dimentions
             */
            resize_bitmap(&img_src, &img_dst);

            /* update decoder struct */
            d->width = img_dst.width;
            d->height = img_dst.height;
            d->native_img_size = c_native_img_size;

            /* copy back corrected image to the begining of the buffer */
            memmove(img_src.data, img_dst.data, d->native_img_size);
        }
    }
#endif
}
