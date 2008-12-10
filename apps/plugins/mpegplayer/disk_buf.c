/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * mpegplayer buffering routines
 *
 * Copyright (c) 2007 Michael Sevakis
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
#include "mpegplayer.h"

static struct mutex disk_buf_mtx SHAREDBSS_ATTR;
static struct event_queue disk_buf_queue SHAREDBSS_ATTR;
static struct queue_sender_list disk_buf_queue_send SHAREDBSS_ATTR;
static uint32_t disk_buf_stack[DEFAULT_STACK_SIZE*2/sizeof(uint32_t)];

struct disk_buf disk_buf SHAREDBSS_ATTR;
static struct list_item nf_list;

static inline void disk_buf_lock(void)
{
    rb->mutex_lock(&disk_buf_mtx);
}

static inline void disk_buf_unlock(void)
{
    rb->mutex_unlock(&disk_buf_mtx);
}

static inline void disk_buf_on_clear_data_notify(struct stream_hdr *sh)
{
    DEBUGF("DISK_BUF_CLEAR_DATA_NOTIFY: 0x%02X (cleared)\n",
            STR_FROM_HEADER(sh)->id);
    list_remove_item(&sh->nf);
}

static int disk_buf_on_data_notify(struct stream_hdr *sh)
{
    DEBUGF("DISK_BUF_DATA_NOTIFY: 0x%02X ", STR_FROM_HEADER(sh)->id);

    if (sh->win_left <= sh->win_right)
    {
        /* Check if the data is already ready already */
        if (disk_buf_is_data_ready(sh, 0))
        {
            /* It was - don't register */
            DEBUGF("(was ready)\n"
                   "  swl:%lu swr:%lu\n"
                   "  dwl:%lu dwr:%lu\n",
                   sh->win_left, sh->win_right,
                   disk_buf.win_left, disk_buf.win_right);
            /* Be sure it's not listed though if multiple requests were made */
            list_remove_item(&sh->nf);
            return DISK_BUF_NOTIFY_OK;
        }

        switch (disk_buf.state)
        {
        case TSTATE_DATA:
        case TSTATE_BUFFERING:
        case TSTATE_INIT:
            disk_buf.state = TSTATE_BUFFERING;
            list_add_item(&nf_list, &sh->nf);
            DEBUGF("(registered)\n"
                   "  swl:%lu swr:%lu\n"
                   "  dwl:%lu dwr:%lu\n",
                   sh->win_left, sh->win_right,
                   disk_buf.win_left, disk_buf.win_right);
            return DISK_BUF_NOTIFY_REGISTERED;
        }
    }

    DEBUGF("(error)\n");
    return DISK_BUF_NOTIFY_ERROR;
}

static bool check_data_notifies_callback(struct list_item *item,
                                         intptr_t data)
{
    struct stream_hdr *sh = TYPE_FROM_MEMBER(struct stream_hdr, item, nf);

    if (disk_buf_is_data_ready(sh, 0))
    {
        /* Remove from list then post notification - post because send
         * could result in a wait for each thread to finish resulting
         * in deadlock */
        list_remove_item(item);
        str_post_msg(STR_FROM_HEADER(sh), DISK_BUF_DATA_NOTIFY, 0);
        DEBUGF("DISK_BUF_DATA_NOTIFY: 0x%02X (notified)\n",
               STR_FROM_HEADER(sh)->id);
    }

    return true;
    (void)data;
}

/* Check registered streams and notify them if their data is available */
static void check_data_notifies(void)
{
    list_enum_items(&nf_list, check_data_notifies_callback, 0);
}

/* Clear all registered notifications - do not post them */
static inline void clear_data_notifies(void)
{
    list_clear_all(&nf_list);
}

/* Background buffering when streaming */
static inline void disk_buf_buffer(void)
{
    struct stream_window sw;

    switch (disk_buf.state)
    {
    default:
    {
        size_t wm;
        uint32_t time;

        /* Get remaining minimum data based upon the stream closest to the
         * right edge of the window */
        if (!stream_get_window(&sw))
            break;

        time = stream_get_ticks(NULL);
        wm = muldiv_uint32(5*CLOCK_RATE, sw.right - disk_buf.pos_last,
                           time - disk_buf.time_last);
        wm = MIN(wm, (size_t)disk_buf.size);
        wm = MAX(wm, DISK_BUF_LOW_WATERMARK);

        disk_buf.time_last = time;
        disk_buf.pos_last = sw.right;

        /* Fast attack, slow decay */
        disk_buf.low_wm = (wm > (size_t)disk_buf.low_wm) ?
            wm : AVERAGE(disk_buf.low_wm, wm, 16);

#if 0
        rb->splashf(0, "*%10ld %10ld", disk_buf.low_wm,
                   disk_buf.win_right - sw.right);
#endif

        if (disk_buf.win_right - sw.right > disk_buf.low_wm)
            break;

        disk_buf.state = TSTATE_BUFFERING;
        } /* default: */

        /* Fall-through */
    case TSTATE_BUFFERING:
    {
        ssize_t len, n;
        uint32_t tag, *tag_p;

        /* Limit buffering up to the stream with the least progress */
        if (!stream_get_window(&sw))
        {
            disk_buf.state = TSTATE_DATA;
            rb->storage_sleep();
            break;
        }

        /* Wrap pointer */
        if (disk_buf.tail >= disk_buf.end)
            disk_buf.tail = disk_buf.start;

        len = disk_buf.size - disk_buf.win_right + sw.left;

        if (len < DISK_BUF_PAGE_SIZE)
        {
            /* Free space is less than one page */
            disk_buf.state = TSTATE_DATA;
            disk_buf.low_wm = DISK_BUF_LOW_WATERMARK;
            rb->storage_sleep();
            break;
        }

        len = disk_buf.tail - disk_buf.start;
        tag = MAP_OFFSET_TO_TAG(disk_buf.win_right);
        tag_p = &disk_buf.cache[len >> DISK_BUF_PAGE_SHIFT];

        if (*tag_p != tag)
        {
            if (disk_buf.need_seek)
            {
                rb->lseek(disk_buf.in_file, disk_buf.win_right, SEEK_SET);
                disk_buf.need_seek = false;
            }

            n = rb->read(disk_buf.in_file, disk_buf.tail, DISK_BUF_PAGE_SIZE);

            if (n <= 0)
            {
                /* Error or end of stream */
                disk_buf.state = TSTATE_EOS;
                rb->storage_sleep();
                break;
            }

            if (len < DISK_GUARDBUF_SIZE)
            {
                /* Autoguard guard-o-rama - maintain guardbuffer coherency */
                rb->memcpy(disk_buf.end + len, disk_buf.tail,
                           MIN(DISK_GUARDBUF_SIZE - len, n));
            }

            /* Update the cache entry for this page */
            *tag_p = tag;
        }
        else
        {
            /* Skipping a read */
            n = MIN(DISK_BUF_PAGE_SIZE,
                    disk_buf.filesize - disk_buf.win_right);
            disk_buf.need_seek = true;
        }

        disk_buf.tail += DISK_BUF_PAGE_SIZE;

        /* Keep left edge moving forward */

        /* Advance right edge in temp variable first, then move
         * left edge if overflow would occur. This avoids a stream
         * thinking its data might be available when it actually
         * may not end up that way after a slide of the window. */
        len = disk_buf.win_right + n;

        if (len - disk_buf.win_left > disk_buf.size)
            disk_buf.win_left += n;

        disk_buf.win_right = len;

        /* Continue buffering until filled or file end */
        rb->yield();
        } /* TSTATE_BUFFERING: */

    case TSTATE_EOS:
        break;
    } /* end switch */
}

static void disk_buf_on_reset(ssize_t pos)
{
    int page;
    uint32_t tag;
    off_t anchor;

    disk_buf.state = TSTATE_INIT;
    disk_buf.status = STREAM_STOPPED;
    clear_data_notifies();

    if (pos >= disk_buf.filesize)
    {
        /* Anchor on page immediately following the one containing final
         * data */
        anchor = disk_buf.file_pages * DISK_BUF_PAGE_SIZE;
        disk_buf.win_left = disk_buf.filesize;
    }
    else
    {
        anchor = pos & ~DISK_BUF_PAGE_MASK;
        disk_buf.win_left = anchor;
    }

    /* Collect all valid data already buffered that is contiguous with the
     * current position - probe to left, then to right */
    if (anchor > 0)
    {
        page = MAP_OFFSET_TO_PAGE(anchor);
        tag = MAP_OFFSET_TO_TAG(anchor);

        do
        {
            if (--tag, --page < 0)
                page = disk_buf.pgcount - 1;
    
            if (disk_buf.cache[page] != tag)
                break;

            disk_buf.win_left = tag << DISK_BUF_PAGE_SHIFT;
        }
        while (tag > 0);
    }

    if (anchor < disk_buf.filesize)
    {
        page = MAP_OFFSET_TO_PAGE(anchor);
        tag = MAP_OFFSET_TO_TAG(anchor);

        do
        {
            if (disk_buf.cache[page] != tag)
                break;

            if (++tag, ++page >= disk_buf.pgcount)
                page = 0;

            anchor += DISK_BUF_PAGE_SIZE;
        }
        while (anchor < disk_buf.filesize);
    }

    if (anchor >= disk_buf.filesize)
    {
        disk_buf.win_right = disk_buf.filesize;
        disk_buf.state = TSTATE_EOS;
    }
    else
    {
        disk_buf.win_right = anchor;
    }

    disk_buf.tail = disk_buf.start + MAP_OFFSET_TO_BUFFER(anchor);

    DEBUGF("disk buf reset\n"
           "  dwl:%ld dwr:%ld\n",
           disk_buf.win_left, disk_buf.win_right);

    /* Next read position is at right edge */
    rb->lseek(disk_buf.in_file, disk_buf.win_right, SEEK_SET);
    disk_buf.need_seek = false;

    disk_buf_reply_msg(disk_buf.win_right - disk_buf.win_left);
}

static void disk_buf_on_stop(void)
{
    bool was_buffering = disk_buf.state == TSTATE_BUFFERING;

    disk_buf.state = TSTATE_EOS;
    disk_buf.status = STREAM_STOPPED;
    clear_data_notifies();

    disk_buf_reply_msg(was_buffering);
}

static void disk_buf_on_play_pause(bool play, bool forcefill)
{
    struct stream_window sw;

    if (disk_buf.state != TSTATE_EOS)
    {
        if (forcefill)
        {
            /* Force buffer filling to top */
            disk_buf.state = TSTATE_BUFFERING;
        }
        else if (disk_buf.state != TSTATE_BUFFERING)
        {
            /* If not filling already, simply monitor */
            disk_buf.state = TSTATE_DATA;
        }
    }
    /* else end of stream - no buffering to do */

    disk_buf.pos_last = stream_get_window(&sw) ? sw.right : 0;
    disk_buf.time_last = stream_get_ticks(NULL);

    disk_buf.status = play ? STREAM_PLAYING : STREAM_PAUSED;
}

static int disk_buf_on_load_range(struct dbuf_range *rng)
{
    uint32_t tag = rng->tag_start;
    uint32_t tag_end = rng->tag_end;
    int page = rng->pg_start;

    /* Check if a seek is required */
    bool need_seek = rb->lseek(disk_buf.in_file, 0, SEEK_CUR)
                        != (off_t)(tag << DISK_BUF_PAGE_SHIFT);

    do
    {
        uint32_t *tag_p = &disk_buf.cache[page];

        if (*tag_p != tag)
        {
            /* Page not cached - load it */
            ssize_t o, n;

            if (need_seek)
            {
                rb->lseek(disk_buf.in_file, tag << DISK_BUF_PAGE_SHIFT,
                          SEEK_SET);
                need_seek = false;
            }

            o = page << DISK_BUF_PAGE_SHIFT;
            n = rb->read(disk_buf.in_file, disk_buf.start + o,
                         DISK_BUF_PAGE_SIZE);

            if (n < 0)
            {
                /* Read error */
                return DISK_BUF_NOTIFY_ERROR;
            }

            if (n == 0)
            {
                /* End of file */
                break;
            }

            if (o < DISK_GUARDBUF_SIZE)
            {
                /* Autoguard guard-o-rama - maintain guardbuffer coherency */
                rb->memcpy(disk_buf.end + o, disk_buf.start + o,
                           MIN(DISK_GUARDBUF_SIZE - o, n));
            }

            /* Update the cache entry */
            *tag_p = tag;
        }
        else
        {
            /* Skipping a disk read - must seek on next one */
            need_seek = true;
        }

        if (++page >= disk_buf.pgcount)
            page = 0;
    }
    while (++tag <= tag_end);

    return DISK_BUF_NOTIFY_OK;
}

static void disk_buf_thread(void)
{
    struct queue_event ev;

    disk_buf.state = TSTATE_EOS;
    disk_buf.status = STREAM_STOPPED;

    while (1)    
    {
        if (disk_buf.state != TSTATE_EOS)
        {
            /* Poll buffer status and messages */
            rb->queue_wait_w_tmo(disk_buf.q, &ev,
                                 disk_buf.state == TSTATE_BUFFERING ?
                                    0 : HZ/5);
        }
        else
        {
            /* Sit idle and wait for commands */
            rb->queue_wait(disk_buf.q, &ev);
        }

        switch (ev.id)
        {
        case SYS_TIMEOUT:
            if (disk_buf.state == TSTATE_EOS)
                break;

            disk_buf_buffer();

            /* Check for any due notifications if any are pending */
            if (nf_list.next != NULL)
                check_data_notifies();

            /* Still more data left? */
            if (disk_buf.state != TSTATE_EOS)
                continue;

            /* Nope - end of stream */
            break;

        case DISK_BUF_CACHE_RANGE:
            disk_buf_reply_msg(disk_buf_on_load_range(
                                (struct dbuf_range *)ev.data));
            break;

        case STREAM_RESET:
            disk_buf_on_reset(ev.data);
            break;

        case STREAM_STOP:
            disk_buf_on_stop();
            break;

        case STREAM_PAUSE:
        case STREAM_PLAY:
            disk_buf_on_play_pause(ev.id == STREAM_PLAY, ev.data != 0);
            disk_buf_reply_msg(1);
            break;

        case STREAM_QUIT:
            disk_buf.state = TSTATE_EOS;
            return;

        case DISK_BUF_DATA_NOTIFY:
            disk_buf_reply_msg(disk_buf_on_data_notify(
                                (struct stream_hdr *)ev.data));
            break;

        case DISK_BUF_CLEAR_DATA_NOTIFY:
            disk_buf_on_clear_data_notify((struct stream_hdr *)ev.data);
            disk_buf_reply_msg(1);
            break;
        }
    }
}

/* Caches some data from the current file */
static int disk_buf_probe(off_t start, size_t length,
                          void **p, size_t *outlen)
{
    off_t end;
    uint32_t tag, tag_end;
    int page;

    /* Can't read past end of file */
    if (length > (size_t)(disk_buf.filesize - disk_buf.offset))
    {
        length = disk_buf.filesize - disk_buf.offset;
    }

    /* Can't cache more than the whole buffer size */
    if (length > (size_t)disk_buf.size)
    {
        length = disk_buf.size;
    }
    /* Zero-length probes permitted */

    end = start + length;

    /* Prepare the range probe */
    tag = MAP_OFFSET_TO_TAG(start);
    tag_end = MAP_OFFSET_TO_TAG(end);
    page = MAP_OFFSET_TO_PAGE(start);

    /* If the end is on a page boundary, check one less or an extra
     * one will be probed */
    if (tag_end > tag && (end & DISK_BUF_PAGE_MASK) == 0)
    {
        tag_end--;
    }

    if (p != NULL)
    {
        *p = disk_buf.start + (page << DISK_BUF_PAGE_SHIFT)
                + (start & DISK_BUF_PAGE_MASK);
    }

    if (outlen != NULL)
    {
        *outlen = length;
    }

    /* Obtain initial load point. If all data was cached, no message is sent
     * otherwise begin on the first page that is not cached. Since we have to
     * send the message anyway, the buffering thread will determine what else
     * requires loading on its end in order to cache the specified range. */
    do
    {
        if (disk_buf.cache[page] != tag)
        {
            static struct dbuf_range rng IBSS_ATTR;
            DEBUGF("disk_buf: cache miss\n");
            rng.tag_start = tag;
            rng.tag_end = tag_end;
            rng.pg_start = page;
            return rb->queue_send(disk_buf.q, DISK_BUF_CACHE_RANGE,
                                  (intptr_t)&rng);
        }

        if (++page >= disk_buf.pgcount)
            page = 0;
    }
    while (++tag <= tag_end);

    return DISK_BUF_NOTIFY_OK;
}

/* Attempt to get a pointer to size bytes on the buffer. Returns real amount of
 * data available as well as the size of non-wrapped data after *p. */
ssize_t _disk_buf_getbuffer(size_t size, void **pp, void **pwrap, size_t *sizewrap)
{
    disk_buf_lock();

    if (disk_buf_probe(disk_buf.offset, size, pp, &size) == DISK_BUF_NOTIFY_OK)
    {
        if (pwrap && sizewrap)
        {
            uint8_t *p = (uint8_t *)*pp;

            if (p + size > disk_buf.end + DISK_GUARDBUF_SIZE)
            {
                /* Return pointer to wraparound and the size of same */
                size_t nowrap = (disk_buf.end + DISK_GUARDBUF_SIZE) - p;
                *pwrap = disk_buf.start + DISK_GUARDBUF_SIZE;
                *sizewrap = size - nowrap;
            }
            else
            {
                *pwrap = NULL;
                *sizewrap = 0;
            }
        }
    }
    else
    {
        size = -1;
    }

    disk_buf_unlock();

    return size;
}

/* Read size bytes of data into a buffer - advances the buffer pointer
 * and returns the real size read. */
ssize_t disk_buf_read(void *buffer, size_t size)
{
    uint8_t *p;

    disk_buf_lock();

    if (disk_buf_probe(disk_buf.offset, size, PUN_PTR(void **, &p),
                       &size) == DISK_BUF_NOTIFY_OK)
    {
        if (p + size > disk_buf.end + DISK_GUARDBUF_SIZE)
        {
            /* Read wraps */
            size_t nowrap = (disk_buf.end + DISK_GUARDBUF_SIZE) - p;
            rb->memcpy(buffer, p, nowrap);
            rb->memcpy(buffer + nowrap, disk_buf.start + DISK_GUARDBUF_SIZE,
                       size - nowrap);
        }
        else
        {
            /* Read wasn't wrapped or guardbuffer holds it */
            rb->memcpy(buffer, p, size);
        }

        disk_buf.offset += size;
    }
    else
    {   
        size = -1;
    }

    disk_buf_unlock();

    return size;
}

off_t disk_buf_lseek(off_t offset, int whence)
{
    disk_buf_lock();

    /* The offset returned is the result of the current thread's action and
     * may be invalidated so a local result is returned and not the value
     * of disk_buf.offset directly */
    switch (whence)
    {
    case SEEK_SET:
        /* offset is just the offset */
        break;
    case SEEK_CUR:
        offset += disk_buf.offset;
        break;
    case SEEK_END:
        offset = disk_buf.filesize + offset;
        break;
    default:
        disk_buf_unlock();
        return -2; /* Invalid request */
    }

    if (offset < 0 || offset > disk_buf.filesize)
    {
        offset = -3;
    }
    else
    {
        disk_buf.offset = offset;
    }

    disk_buf_unlock();

    return offset;
}

/* Prepare the buffer to enter the streaming state. Evaluates the available
 * streaming window. */
ssize_t disk_buf_prepare_streaming(off_t pos, size_t len)
{
    disk_buf_lock();

    if (pos < 0)
        pos = 0;
    else if (pos > disk_buf.filesize)
        pos = disk_buf.filesize;

    DEBUGF("prepare streaming:\n  pos:%ld len:%lu\n", pos, len);

    pos = disk_buf_lseek(pos, SEEK_SET);
    disk_buf_probe(pos, len, NULL, &len);

    DEBUGF("  probe done: pos:%ld len:%lu\n", pos, len);

    len = disk_buf_send_msg(STREAM_RESET, pos);

    disk_buf_unlock();

    return len;
}

/* Set the streaming window to an arbitrary position within the file. Makes no
 * probes to validate data. Use after calling another function to cause data
 * to be cached and correct values are known. */
ssize_t disk_buf_set_streaming_window(off_t left, off_t right)
{
    ssize_t len;

    disk_buf_lock();

    if (left < 0)
        left = 0;
    else if (left > disk_buf.filesize)
        left = disk_buf.filesize;

    if (left > right)
        right = left;

    if (right > disk_buf.filesize)
        right = disk_buf.filesize;

    disk_buf.win_left = left;
    disk_buf.win_right = right;
    disk_buf.tail = disk_buf.start + ((right + DISK_BUF_PAGE_SIZE-1) &
                                       ~DISK_BUF_PAGE_MASK) % disk_buf.size;

    len = disk_buf.win_right - disk_buf.win_left;

    disk_buf_unlock();

    return len;
}

void * disk_buf_offset2ptr(off_t offset)
{
    if (offset < 0)
        offset = 0;
    else if (offset > disk_buf.filesize)
        offset = disk_buf.filesize;

    return disk_buf.start + (offset % disk_buf.size);
}

void disk_buf_close(void)
{
    disk_buf_lock();

    if (disk_buf.in_file >= 0)
    {
        rb->close(disk_buf.in_file);
        disk_buf.in_file = -1;

        /* Invalidate entire cache */
        rb->memset(disk_buf.cache, 0xff,
                   disk_buf.pgcount*sizeof (*disk_buf.cache));
        disk_buf.file_pages = 0;
        disk_buf.filesize = 0;
        disk_buf.offset = 0;
    }

    disk_buf_unlock();
}

int disk_buf_open(const char *filename)
{
    int fd;

    disk_buf_lock();

    disk_buf_close();

    fd = rb->open(filename, O_RDONLY);

    if (fd >= 0)
    {
        ssize_t filesize = rb->filesize(fd);

        if (filesize <= 0)
        {
            rb->close(disk_buf.in_file);
        }
        else
        {
            disk_buf.filesize = filesize;
            /* Number of file pages rounded up toward +inf */
            disk_buf.file_pages = ((size_t)filesize + DISK_BUF_PAGE_SIZE-1)
                                    / DISK_BUF_PAGE_SIZE;
            disk_buf.in_file = fd;
        }
    }

    disk_buf_unlock();

    return fd;
}

intptr_t disk_buf_send_msg(long id, intptr_t data)
{
    return rb->queue_send(disk_buf.q, id, data);
}

void disk_buf_post_msg(long id, intptr_t data)
{
    rb->queue_post(disk_buf.q, id, data);
}

void disk_buf_reply_msg(intptr_t retval)
{
    rb->queue_reply(disk_buf.q, retval);
}

bool disk_buf_init(void)
{
    disk_buf.thread = 0;
    list_initialize(&nf_list);

    rb->mutex_init(&disk_buf_mtx);

    disk_buf.q = &disk_buf_queue;
    rb->queue_init(disk_buf.q, false);

    disk_buf.state  = TSTATE_EOS;
    disk_buf.status = STREAM_STOPPED;

    disk_buf.in_file = -1;
    disk_buf.filesize = 0;
    disk_buf.win_left = 0;
    disk_buf.win_right = 0;
    disk_buf.time_last = 0;
    disk_buf.pos_last = 0;
    disk_buf.low_wm = DISK_BUF_LOW_WATERMARK;

    disk_buf.start = mpeg_malloc_all(&disk_buf.size, MPEG_ALLOC_DISKBUF);
    if (disk_buf.start == NULL)
        return false;

#ifdef PROC_NEEDS_CACHEALIGN
    disk_buf.size = CACHEALIGN_BUFFER(&disk_buf.start, disk_buf.size);
    disk_buf.start = UNCACHED_ADDR(disk_buf.start);
#endif
    disk_buf.size -= DISK_GUARDBUF_SIZE;
    disk_buf.pgcount = disk_buf.size / DISK_BUF_PAGE_SIZE;

    /* Fit it as tightly as possible */
    while (disk_buf.pgcount*(sizeof (*disk_buf.cache) + DISK_BUF_PAGE_SIZE)
            > (size_t)disk_buf.size)
    {
        disk_buf.pgcount--;
    }

    disk_buf.cache = (typeof (disk_buf.cache))disk_buf.start;
    disk_buf.start += sizeof (*disk_buf.cache)*disk_buf.pgcount;
    disk_buf.size = disk_buf.pgcount*DISK_BUF_PAGE_SIZE;
    disk_buf.end = disk_buf.start + disk_buf.size;
    disk_buf.tail = disk_buf.start;

    DEBUGF("disk_buf info:\n"
           "  page count: %d\n"
           "  size:       %ld\n",
           disk_buf.pgcount, disk_buf.size);

    rb->memset(disk_buf.cache, 0xff,
               disk_buf.pgcount*sizeof (*disk_buf.cache));

    disk_buf.thread = rb->create_thread(
        disk_buf_thread, disk_buf_stack, sizeof(disk_buf_stack), 0,
        "mpgbuffer" IF_PRIO(, PRIORITY_BUFFERING) IF_COP(, CPU));

    rb->queue_enable_queue_send(disk_buf.q, &disk_buf_queue_send,
                                disk_buf.thread);

    if (disk_buf.thread == 0)
        return false;

    /* Wait for thread to initialize */
    disk_buf_send_msg(STREAM_NULL, 0);

    return true;
}

void disk_buf_exit(void)
{
    if (disk_buf.thread != 0)
    {
        rb->queue_post(disk_buf.q, STREAM_QUIT, 0);
        rb->thread_wait(disk_buf.thread);
        disk_buf.thread = 0;
    }
}
