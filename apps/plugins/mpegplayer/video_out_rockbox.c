/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * mpegplayer video output routines
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "mpeg2dec_config.h"

#include "plugin.h"
#include "mpegplayer.h"

struct vo_data
{
    int image_width;
    int image_height;
    int image_chroma_x;
    int image_chroma_y;
    int display_width;
    int display_height;
    int output_x;
    int output_y;
    int output_width;
    int output_height;
    bool visible;
    bool thumb_mode;
    void *last;
};

#ifdef PROC_NEEDS_CACHEALIGN
/* Cache aligned and padded to avoid clobbering other processors' cacheable
 * data */
static uint8_t __vo_data[CACHEALIGN_UP(sizeof(struct vo_data))]
        CACHEALIGN_ATTR;
#define vo (*((struct vo_data *)__vo_data))
#else
static struct vo_data vo;
#endif

/* Draw a black rectangle if no video frame is available */
static void vo_draw_black(void)
{
    int foreground = lcd_(get_foreground)();

    lcd_(set_foreground)(DRAW_BLACK);

    lcd_(fillrect)(vo.output_x, vo.output_y, vo.output_width,
                   vo.output_height);
    lcd_(update_rect)(vo.output_x, vo.output_y, vo.output_width,
                      vo.output_height);

    lcd_(set_foreground)(foreground);
}

static inline void yuv_blit(uint8_t * const * buf, int src_x, int src_y,
                            int stride, int x, int y, int width, int height)
{
#ifdef HAVE_LCD_COLOR
    rb->lcd_yuv_blit(buf, src_x, src_y, stride, x, y , width, height);
#else
    gray_ub_gray_bitmap_part(buf[0], src_x, src_y, stride, x, y, width, height);
#endif
}

void vo_draw_frame(uint8_t * const * buf)
{
    if (!vo.visible)
    {
        /* Frame is hidden - copout */
        DEBUGF("vo hidden\n");
        return;
    }
    else if (buf == NULL)
    {
        /* No frame exists - draw black */
        vo_draw_black();
        DEBUGF("vo no frame\n");
        return;
    }

    yuv_blit(buf, 0, 0, vo.image_width,
             vo.output_x, vo.output_y, vo.output_width,
             vo.output_height);
}

#if LCD_WIDTH >= LCD_HEIGHT
#define SCREEN_WIDTH LCD_WIDTH
#define SCREEN_HEIGHT LCD_HEIGHT
#else /* Assume the screen is rotated on portrait LCDs */
#define SCREEN_WIDTH LCD_HEIGHT
#define SCREEN_HEIGHT LCD_WIDTH
#endif

static inline void vo_rect_clear_inl(struct vo_rect *rc)
{
    rc->l = rc->t = rc->r = rc->b = 0;
}

static inline bool vo_rect_empty_inl(const struct vo_rect *rc)
{
    return rc == NULL || rc->l >= rc->r || rc->t >= rc->b;
}

static inline bool vo_rects_intersect_inl(const struct vo_rect *rc1,
                                          const struct vo_rect *rc2)
{
    return !vo_rect_empty_inl(rc1) &&
           !vo_rect_empty_inl(rc2) &&
           rc1->l < rc2->r && rc1->r > rc2->l &&
           rc1->t < rc2->b && rc1->b > rc2->t;
}

/* Sets all coordinates of a vo_rect to 0 */
void vo_rect_clear(struct vo_rect *rc)
{
    vo_rect_clear_inl(rc);
}

/* Returns true if left >= right or top >= bottom */
bool vo_rect_empty(const struct vo_rect *rc)
{
    return vo_rect_empty_inl(rc);
}

/* Initializes a vo_rect using upper-left corner and extents */
void vo_rect_set_ext(struct vo_rect *rc, int x, int y,
                     int width, int height)
{
    rc->l = x;
    rc->t = y;
    rc->r = x + width;
    rc->b = y + height;
}

/* Query if two rectangles intersect */
bool vo_rects_intersect(const struct vo_rect *rc1,
                        const struct vo_rect *rc2)
{
    return vo_rects_intersect_inl(rc1, rc2);
}

/* Intersect two rectangles, placing the result in rc_dst */
bool vo_rect_intersect(struct vo_rect *rc_dst,
                       const struct vo_rect *rc1,
                       const struct vo_rect *rc2)
{
    if (rc_dst != NULL)
    {
        if (vo_rects_intersect_inl(rc1, rc2))
        {
            rc_dst->l = MAX(rc1->l, rc2->l);
            rc_dst->r = MIN(rc1->r, rc2->r);
            rc_dst->t = MAX(rc1->t, rc2->t);
            rc_dst->b = MIN(rc1->b, rc2->b);
            return true;
        }

        vo_rect_clear_inl(rc_dst);
    }

    return false;
}

/* Shink or stretch each axis - rotate counter-clockwise to retain upright
 * orientation on rotated displays (they rotate clockwise) */
void stretch_image_plane(const uint8_t * src, uint8_t *dst, int stride,
                         int src_w, int src_h, int dst_w, int dst_h)
{
    uint8_t *dst_end = dst + dst_w*dst_h;

#if LCD_WIDTH >= LCD_HEIGHT
    int src_w2 = src_w*2;        /* 2x dimensions (for rounding before division) */
    int dst_w2 = dst_w*2;
    int src_h2 = src_h*2;
    int dst_h2 = dst_h*2;
    int qw = src_w2 / dst_w2;    /* src-dst width ratio quotient */
    int rw = src_w2 - qw*dst_w2; /* src-dst width ratio remainder */
    int qh = src_h2 / dst_h2;    /* src-dst height ratio quotient */
    int rh = src_h2 - qh*dst_h2; /* src-dst height ratio remainder */
    int dw = dst_w;              /* Width error accumulator  */
    int dh = dst_h;              /* Height error accumulator */
#else
    int src_w2 = src_w*2;
    int dst_w2 = dst_h*2;
    int src_h2 = src_h*2;
    int dst_h2 = dst_w*2;
    int qw = src_h2 / dst_w2;
    int rw = src_h2 - qw*dst_w2;
    int qh = src_w2 / dst_h2;
    int rh = src_w2 - qh*dst_h2;
    int dw = dst_h;
    int dh = dst_w;

    src += src_w - 1;
#endif

    while (1)
    {
        const uint8_t *s = src;
#if LCD_WIDTH >= LCD_HEIGHT
        uint8_t * const dst_line_end = dst + dst_w;
#else
        uint8_t * const dst_line_end = dst + dst_h;
#endif
        while (1)
        {
            *dst++ = *s;

            if (dst >= dst_line_end)
            {
                dw = dst_w;
                break;
            }

#if LCD_WIDTH >= LCD_HEIGHT
            s += qw;
#else
            s += qw*stride;
#endif
            dw += rw;

            if (dw >= dst_w2)
            {
                dw -= dst_w2;
#if LCD_WIDTH >= LCD_HEIGHT
                s++;
#else
                s += stride;
#endif
            }
        }

        if (dst >= dst_end)
            break;
#if LCD_WIDTH >= LCD_HEIGHT
        src += qh*stride;
#else
        src -= qh;
#endif
        dh += rh;

        if (dh >= dst_h2)
        {
            dh -= dst_h2;
#if LCD_WIDTH >= LCD_HEIGHT
            src += stride;
#else
            src--;
#endif
        }
    }
}

bool vo_draw_frame_thumb(uint8_t * const * buf, const struct vo_rect *rc)
{
    void *mem;
    size_t bufsize;
    uint8_t *yuv[3];
    struct vo_rect thumb_rc;
    int thumb_width, thumb_height;
    int thumb_uv_width, thumb_uv_height;

    if (buf == NULL)
        return false;

    /* Obtain rectangle as clipped to the screen */
    vo_rect_set_ext(&thumb_rc, 0, 0, LCD_WIDTH, LCD_HEIGHT);
    if (!vo_rect_intersect(&thumb_rc, rc, &thumb_rc))
        return true;

    DEBUGF("thumb_rc: %d, %d, %d, %d\n", thumb_rc.l, thumb_rc.t,
           thumb_rc.r, thumb_rc.b);

    thumb_width = rc->r - rc->l;
    thumb_height = rc->b - rc->t;
    thumb_uv_width = thumb_width / 2;
    thumb_uv_height = thumb_height / 2;

    DEBUGF("thumb: w: %d h: %d uvw: %d uvh: %d\n", thumb_width,
           thumb_height, thumb_uv_width, thumb_uv_height);

    /* Use remaining mpeg2 buffer as temp space */
    mem = mpeg2_get_buf(&bufsize);

    if (bufsize < (size_t)(thumb_width*thumb_height)
#ifdef HAVE_LCD_COLOR
            + 2u*(thumb_uv_width * thumb_uv_height)
#endif
            )
    {
        DEBUGF("thumb: insufficient buffer\n");
        return false;
    }

    yuv[0] = mem;
    stretch_image_plane(buf[0], yuv[0], vo.image_width,
                        vo.display_width, vo.display_height,
                        thumb_width, thumb_height);

#ifdef HAVE_LCD_COLOR
    yuv[1] = yuv[0] + thumb_width*thumb_height;
    yuv[2] = yuv[1] + thumb_uv_width*thumb_uv_height;

    stretch_image_plane(buf[1], yuv[1], vo.image_width / 2,
                        vo.display_width / 2, vo.display_height / 2,
                        thumb_uv_width, thumb_uv_height);

    stretch_image_plane(buf[2], yuv[2], vo.image_width / 2,
                        vo.display_width / 2, vo.display_height / 2,
                        thumb_uv_width, thumb_uv_height);
#endif

#if LCD_WIDTH >= LCD_HEIGHT
    yuv_blit(yuv, 0, 0, thumb_width,
             thumb_rc.l, thumb_rc.t,
             thumb_rc.r - thumb_rc.l,
             thumb_rc.b - thumb_rc.t);
#else
    yuv_blit(yuv, 0, 0, thumb_height,
             thumb_rc.t, thumb_rc.l,
             thumb_rc.b - thumb_rc.t,
             thumb_rc.r - thumb_rc.l);
#endif /* LCD_WIDTH >= LCD_HEIGHT */

    return true;
}

void vo_setup(const mpeg2_sequence_t * sequence)
{
    vo.image_width = sequence->width;
    vo.image_height = sequence->height;
    vo.display_width = sequence->display_width;
    vo.display_height = sequence->display_height;

    DEBUGF("vo_setup - w:%d h:%d\n", vo.display_width, vo.display_height);

    vo.image_chroma_x = vo.image_width / sequence->chroma_width;
    vo.image_chroma_y = vo.image_height / sequence->chroma_height;

    if (sequence->display_width >= SCREEN_WIDTH)
    {
        vo.output_width = SCREEN_WIDTH;
        vo.output_x = 0;
    }
    else
    {
        vo.output_width = sequence->display_width;
        vo.output_x = (SCREEN_WIDTH - sequence->display_width) / 2;
    }

    if (sequence->display_height >= SCREEN_HEIGHT)
    {
        vo.output_height = SCREEN_HEIGHT;
        vo.output_y = 0;
    }
    else
    {
        vo.output_height = sequence->display_height;
        vo.output_y = (SCREEN_HEIGHT - sequence->display_height) / 2;
    }
}

void vo_dimensions(struct vo_ext *sz)
{
    sz->w = vo.display_width;
    sz->h = vo.display_height;
}

bool vo_init(void)
{
    vo.visible = false;
    return true;
}

bool vo_show(bool show)
{
    bool vis = vo.visible;
    vo.visible = show;
    return vis;
}

bool vo_is_visible(void)
{
    return vo.visible;
}

void vo_cleanup(void)
{
    vo.visible = false;
#ifndef HAVE_LCD_COLOR
    gray_release();
#endif
}
