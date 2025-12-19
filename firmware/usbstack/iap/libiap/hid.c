#include <string.h>

#include "constants.h"
#include "iap.h"
#include "macros.h"
#include "platform.h"
#include "spec/hid.h"

struct ReportSize {
    uint8_t  id;
    uint16_t size; /* including link control byte */
};

/* must match to in-driver hid report descriptor */

static struct ReportSize input_report_size_table_fs[] = {
    {.id = 0x01, .size = 0x0C},
    {.id = 0x02, .size = 0x0E},
    {.id = 0x03, .size = 0x14},
    {.id = 0x04, .size = 0x3F},
};

static struct ReportSize output_report_size_table_fs[] = {
    {.id = 0x05, .size = 0x08},
    {.id = 0x06, .size = 0x0A},
    {.id = 0x07, .size = 0x0E},
    {.id = 0x08, .size = 0x14},
    {.id = 0x09, .size = 0x3F},
};

static struct ReportSize input_report_size_table_hs[] = {
    {.id = 0x01, .size = 0x0005},
    {.id = 0x02, .size = 0x0009},
    {.id = 0x03, .size = 0x000D},
    {.id = 0x04, .size = 0x0011},
    {.id = 0x05, .size = 0x0019},
    {.id = 0x06, .size = 0x0031},
    {.id = 0x07, .size = 0x005F},
    {.id = 0x08, .size = 0x00C1},
    {.id = 0x09, .size = 0x0101},
    {.id = 0x0A, .size = 0x0181},
    {.id = 0x0B, .size = 0x0201},
    {.id = 0x0C, .size = 0x02FF},
};

static struct ReportSize output_report_size_table_hs[] = {
    {.id = 0x0D, .size = 0x05},
    {.id = 0x0E, .size = 0x09},
    {.id = 0x1F, .size = 0x0D},
    {.id = 0x10, .size = 0x11},
    {.id = 0x11, .size = 0x19},
    {.id = 0x12, .size = 0x31},
    {.id = 0x13, .size = 0x5F},
    {.id = 0x14, .size = 0xC1},
    {.id = 0x15, .size = 0xFF},
};

static int find_output_report_size(struct IAPContext* ctx, uint8_t id) {
    const struct ReportSize* ptr = ctx->opts.usb_highspeed ? output_report_size_table_hs : output_report_size_table_fs;
    const size_t             len = ctx->opts.usb_highspeed ? array_size(output_report_size_table_hs) : array_size(output_report_size_table_fs);
    for(size_t i = 0; i < len; i += 1) {
        if(ptr[i].id == id) {
            return ptr[i].size;
        }
    }
    return -1;
}

IAPBool iap_feed_hid_report(struct IAPContext* ctx, const uint8_t* const data, const size_t size) {
    check_ret(size > sizeof(struct IAPHIDReport), iap_false);
    struct IAPHIDReport* report = (struct IAPHIDReport*)data;

    int report_size;
    if(ctx->opts.ignore_hid_report_id) {
        report_size = size - 1;
    } else {
        report_size = find_output_report_size(ctx, report->report_id);
        check_ret(report_size == (int)size - 1, iap_false, "%d != %d", report_size, (int)size - 1);
    }

    const uint8_t payload_size = report_size - 1;
    check_act(ctx->hid_recv_buf_cursor + payload_size <= HID_BUFFER_SIZE, { ctx->hid_recv_buf_cursor = 0; return iap_false; }, "hid buffer overflow");
    if(!(report->link_control & IAPHIDReportLinkControlBits_Continue)) {
        /* not continue, first packet */
        if(ctx->hid_recv_buf_cursor != 0) {
            warn("not continue and cursor was set");
            ctx->hid_recv_buf_cursor = 0;
        }
    } else {
        check_ret(ctx->hid_recv_buf_cursor > 0, iap_false);
    }
    memcpy(ctx->hid_recv_buf + ctx->hid_recv_buf_cursor, report->data, payload_size);
    ctx->hid_recv_buf_cursor += payload_size;
    if(!(report->link_control & IAPHIDReportLinkControlBits_MoreToFollow)) {
        /* no more to follow, last packet */
        const IAPBool ret        = _iap_feed_packet(ctx, ctx->hid_recv_buf, ctx->hid_recv_buf_cursor);
        ctx->hid_recv_buf_cursor = 0;
        check_ret(ret, iap_false);
    }
    return iap_true;
}

IAPBool iap_notify_send_complete(struct IAPContext* ctx) {
    ctx->send_busy = iap_false;
    check_ret(_iap_send_next_report(ctx), iap_false);
    return iap_true;
}

IAPBool _iap_send_hid_reports(struct IAPContext* ctx, size_t begin, size_t end) {
    if(ctx->send_buf_sending_cursor < ctx->send_buf_sending_range_end) {
        warn("another transmission in progress, aborting it");
    }
    ctx->send_buf_sending_cursor      = begin;
    ctx->send_buf_sending_range_begin = begin;
    ctx->send_buf_sending_range_end   = end;
    if(!ctx->send_busy) {
        check_ret(_iap_send_next_report(ctx), iap_false);
    }
    return iap_true;
}

static const struct ReportSize* find_optimal_report_size(struct IAPContext* ctx, size_t size) {
    const struct ReportSize* ptr = ctx->opts.usb_highspeed ? input_report_size_table_hs : input_report_size_table_fs;
    const size_t             len = ctx->opts.usb_highspeed ? array_size(input_report_size_table_hs) : array_size(input_report_size_table_fs);
    for(size_t i = 0; i < len; i += 1) {
        if(ptr[i].size >= size + 1 /* link control byte*/) {
            return &ptr[i];
        }
    }
    return &ptr[len - 1];
}

IAPBool _iap_send_next_report(struct IAPContext* ctx) {
    if(ctx->send_buf_sending_cursor >= ctx->send_buf_sending_range_end) {
        if(ctx->on_send_complete != NULL) {
            IAPOnSendComplete cb  = ctx->on_send_complete;
            ctx->on_send_complete = NULL;
            check_ret(cb(ctx), iap_false);
        } else if(ctx->active_events[0].callback != NULL) {
            struct IAPActiveEvent event = ctx->active_events[0];
            /* shift queue */
            size_t i = 0;
            for(; i < array_size(ctx->active_events) - 1 && ctx->active_events[i + 1].callback != NULL; i += 1) {
                ctx->active_events[i] = ctx->active_events[i + 1];
            }
            ctx->active_events[i].callback = NULL;
            /* process event */
            check_ret(event.callback(ctx, &event), iap_false);
        } else if(ctx->flushing_notifications) {
            check_ret(_iap_flush_notification(ctx), iap_false);
        }
        return iap_true;
    }

    check_ret(!ctx->send_busy, iap_false);

    const size_t                   send_buf_left = ctx->send_buf_sending_range_end - ctx->send_buf_sending_cursor;
    const struct ReportSize* const report_size   = find_optimal_report_size(ctx, send_buf_left);
    const size_t                   take_size     = min((size_t)report_size->size - 1 /* link control */, send_buf_left);

    struct IAPHIDReport* const report = (struct IAPHIDReport*)ctx->hid_send_staging_buf;

    report->report_id = report_size->id;

    const IAPBool is_first = ctx->send_buf_sending_cursor == ctx->send_buf_sending_range_begin;
    const IAPBool is_last  = ctx->send_buf_sending_cursor + take_size == ctx->send_buf_sending_range_end;
    report->link_control =
        (!is_first ? IAPHIDReportLinkControlBits_Continue : 0) |
        (!is_last ? IAPHIDReportLinkControlBits_MoreToFollow : 0);

    memcpy(report->data, ctx->send_buf + ctx->send_buf_sending_cursor, take_size);
    memset(report->data + take_size, 0, report_size->size - 1 - take_size); /* clear rest */

    ctx->send_buf_sending_cursor += take_size;
    ctx->send_busy = iap_true;

    const size_t send_size = 1 + report_size->size;
    check_ret(iap_platform_send_hid_report(ctx, report, send_size) == (int)send_size, iap_false);

    return iap_true;
}
