#include "jpeg81.h"
#include "idct.h"
#include "GETC.h"
#include "rb_glue.h"

#include "../imageviewer.h"


/**************** begin Application ********************/

/************************* Types ***************************/

struct t_disp
{
    unsigned char* bitmap;
};

/************************* Globals ***************************/

/* decompressed image in the possible sizes (1,2,4,8), wasting the other */
static struct t_disp disp[9];

static struct JPEGD jpg; /* too large for stack */

/************************* Implementation ***************************/

static void draw_image_rect(struct image_info *info,
                            int x, int y, int width, int height)
{
    struct t_disp* pdisp = (struct t_disp*)info->data;
#ifdef HAVE_LCD_COLOR
    rb->lcd_bitmap_part(
        (fb_data*)pdisp->bitmap, info->x + x, info->y + y,
        STRIDE(SCREEN_MAIN, info->width, info->height),
        x + MAX(0, (LCD_WIDTH-info->width)/2),
        y + MAX(0, (LCD_HEIGHT-info->height)/2),
        width, height);
#else
    mylcd_ub_gray_bitmap_part(
            pdisp->bitmap, info->x + x, info->y + y, info->width,
            x + MAX(0, (LCD_WIDTH-info->width)/2),
            y + MAX(0, (LCD_HEIGHT-info->height)/2),
            width, height);
#endif
}

static int img_mem(int ds)
{
    struct JPEGD* j = &jpg;
    return j->Y/ds * j->X/ds*sizeof(fb_data);
}

/* my memory pool (from the mp3 buffer) */
static char print[32]; /* use a common snprintf() buffer */

static void scaled_dequantization_and_idct(void)
{
    struct JPEGD* j = &jpg;
    // The following code is based on RAINBOW lib jpeg2bmp example:
    // https://github.com/Halicery/vc_rainbow/blob/605c045a564dad8e2df84e48914eac3d2d8d4a9b/jpeg2bmp.c

    printf("Scaled de-quantization and IDCT.. ");
    int c, i, n;

    // Pre-scale quant-tables
    int SQ[4][64];
    for (c=0; c<4 && j->QT[c][0]; c++)
    {
        int *q= j->QT[c], *sq= SQ[c];
        for (i=0; i<64; i++) sq[i]= q[i] * SCALEM[zigzag[i]];
    }

    // DEQUANT + IDCT
    for (c=0; c<j->Nf; c++)
    {
        struct COMP *C= j->Components+c;
        //int *q= j->QT[C->Qi];
        int *sq= SQ[C->Qi];

        for (n=0; n < C->du_size; n++)
        {
            /*
            // <--- scaled idct
            int k, t[64];
            TCOEF *coef= du[x];
            t[0]= coef[0] * q[0] + 1024;							// dequant DC and level-shift (8-bit)
            for (k=1; k<64; k++) t[zigzag[k]] = coef[k] * q[k];		// dequant AC (+zigzag)
            idct_s(t, coef);
            */

            // <--- scaled idct with dequant
            idct_sq( C->du[ (n / C->du_w) * C->du_width +  n % C->du_w ], sq );
        }
    }
    printf("done\n");
}

static int load_image(char *filename, struct image_info *info,
                      unsigned char *buf, ssize_t *buf_size,
                      int offset, int filesize)
{
    (void)filesize;
    int status;
    struct JPEGD *p_jpg = &jpg;

    memset(&disp, 0, sizeof(disp));
    memset(&jpg, 0, sizeof(jpg));

    init_mem_pool(buf, *buf_size);

    if (!OPEN(filename))
    {
        return PLUGIN_ERROR;
    }
    if (offset)
    {
        POS(offset);
    }

    if (!iv->running_slideshow)
    {
        rb->lcd_puts(0, 0, rb->strrchr(filename,'/')+1);
        rb->lcd_puts(0, 2, "decoding...");
        rb->lcd_update();
    }
    long time; /* measured ticks */

    /* the actual decoding */
    time = *rb->current_tick;
    status = JPEGDecode(p_jpg);
    time = *rb->current_tick - time;

    CLOSE();

    if (status < 0)
    {   /* bad format or minimum components not contained */
        if (status == JPEGENUMERR_MALLOC)
        {
            return PLUGIN_OUTOFMEM;
        }
        rb->splashf(HZ, "unsupported %d", status);
        return  PLUGIN_ERROR;
    }

    if (!iv->running_slideshow)
    {
        rb->lcd_putsf(0, 2, "image %dx%d", p_jpg->X, p_jpg->Y);
        int w, h; /* used to center output */
        rb->snprintf(print, sizeof(print), "jpegp %ld.%02ld sec ", time/HZ, time%HZ);
        rb->lcd_getstringsize(print, &w, &h); /* centered in progress bar */
        rb->lcd_putsxy((LCD_WIDTH - w)/2, LCD_HEIGHT - h, print);
        rb->lcd_update();
        //rb->sleep(100);
    }

    info->x_size = p_jpg->X;
    info->y_size = p_jpg->Y;

#ifdef DISK_SPINDOWN
    if (iv->running_slideshow && iv->immediate_ata_off)
    {
        /* running slideshow and time is long enough: power down disk */
        rb->storage_sleep();
    }
#endif

    if ( 3 != p_jpg->Nf )
        return PLUGIN_ERROR;

    scaled_dequantization_and_idct();

    *buf_size = freeze_mem_pool();
    return PLUGIN_OK;
}

static int get_image(struct image_info *info, int frame, int ds)
{
    (void)frame;
    struct JPEGD* p_jpg = &jpg;
    struct t_disp* p_disp = &disp[ds]; /* short cut */

    info->width = p_jpg->X / ds;
    info->height = p_jpg->Y / ds;
    info->data = p_disp;

    if (p_disp->bitmap != NULL)
    {
        /* we still have it */
        return PLUGIN_OK;
    }

    struct JPEGD* j = p_jpg;
    int mem = img_mem(ds);

    p_disp->bitmap = malloc(mem);

    if (!p_disp->bitmap)
    {
        clear_mem_pool();
        memset(&disp, 0, sizeof(disp));
        p_disp->bitmap = malloc(mem);
        if (!p_disp->bitmap)
            return PLUGIN_ERROR;
    }

    fb_data *bmp = (fb_data *)p_disp->bitmap;

    // The following code is based on RAINBOW lib jpeg2bmp example:
    // https://github.com/Halicery/vc_rainbow/blob/605c045a564dad8e2df84e48914eac3d2d8d4a9b/jpeg2bmp.c
    // Primitive yuv-rgb converter for all sub-sampling types, 24-bit BMP only
    printf("YUV-to-RGB conversion.. ");
    int h0 = j->Hmax / j->Components[0].Hi;
    int v0 = j->Vmax / j->Components[0].Vi;
    int h1 = j->Hmax / j->Components[1].Hi;
    int v1 = j->Vmax / j->Components[1].Vi;
    int h2 = j->Hmax / j->Components[2].Hi;
    int v2 = j->Vmax / j->Components[2].Vi;

    int x, y;
    for (y = 0; y < j->Y; y++)
    {
        if (y%ds != 0)
            continue;

        TCOEF *C0 =
                j->Components[0].du[j->Components[0].du_width * ((y / v0) / 8)] + 8 * ((y / v0) & 7);
        TCOEF *C1 =
                j->Components[1].du[j->Components[1].du_width * ((y / v1) / 8)] + 8 * ((y / v1) & 7);
        TCOEF *C2 =
                j->Components[2].du[j->Components[2].du_width * ((y / v2) / 8)] + 8 * ((y / v2) & 7);

        for (x = 0; x < j->X; x++)
        {
            if (x%ds != 0)
                continue;

            TCOEF c0 = C0[(x / h0 / 8) * 64 + ((x / h0) & 7)];
            TCOEF c1 = C1[(x / h1 / 8) * 64 + ((x / h1) & 7)];
            TCOEF c2 = C2[(x / h2 / 8) * 64 + ((x / h2) & 7)];

            // ITU BT.601 full-range YUV-to-RGB integer approximation 
            {
                int y = (c0 << 5) + 16;
                int u = c1 - 128;
                int v = c2 - 128;

                int b = CLIP[(y + 57 * u)>>5];			// B;
                int g = CLIP[(y - 11 * u - 23 * v)>>5];	// G
                int r = CLIP[(y + 45 * v)>>5];			// R;
                *bmp++= FB_RGBPACK(r,g,b);
            }
        }
    }
    printf("done\n");
    return 0;
}

const struct image_decoder image_decoder = {
    false,
    img_mem,
    load_image,
    get_image,
    draw_image_rect,
};

IMGDEC_HEADER
