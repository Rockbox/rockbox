#pragma once
#include <stddef.h>
#include <stdint.h>

#include "bool.h"
#include "notification.h"
#include "platform.h"

struct IAPContext;
struct IAPSpan;

typedef IAPBool (*IAPOnSendComplete)(struct IAPContext* ctx);

struct IAPOpts {
    /* indicate usb transport is highspeed */
    IAPBool usb_highspeed : 1;
    /* needed to support some accessories which don't set correct hid report id */
    IAPBool ignore_hid_report_id : 1;
    /* limit packet size while sending artworks for stability */
    IAPBool artwork_single_report : 1;
    /* dump each packets */
    IAPBool enable_packet_dump : 1;
};

struct IAPContextButtons {
    IAPBool play_pause : 1;
    IAPBool volume_up : 1;
    IAPBool volume_down : 1;
    IAPBool next_track : 1;
    IAPBool prev_track : 1;
    IAPBool next_album : 1;
    IAPBool prev_album : 1;
    IAPBool stop : 1;
    IAPBool play : 1;
    IAPBool pause : 1;
    IAPBool mute_toggle : 1;
    IAPBool next_chapter : 1;
    IAPBool prev_chapter : 1;
    IAPBool next_playlist : 1;
    IAPBool prev_playlist : 1;
    IAPBool shuffle_advance : 1;
    IAPBool repeat_advance : 1;
};

struct IAPActiveEvent;

typedef IAPBool (*IAPActiveEventReadyCallback)(struct IAPContext* ctx, struct IAPActiveEvent* event);

struct IAPActiveEvent {
    IAPActiveEventReadyCallback callback;
    union {
        uint32_t sampr;
        struct {
            struct IAPPlatformPendingControl control;
            IAPBool                          result;
        } control_response;
    };
};

struct IAPContext {
    /* set by user */
    void*          platform; /* opaque to platform functions */
    struct IAPOpts opts;     /* options */

    /* iap_feed_hid_report */
    uint8_t* hid_recv_buf;
    size_t   hid_recv_buf_cursor;
    /* _iap_send_packet, _iap_send_hid_reports */
    uint8_t* send_buf;
    size_t   send_buf_sending_cursor;
    size_t   send_buf_sending_range_begin;
    size_t   send_buf_sending_range_end;
    /* _iap_send_next_report */
    /* for library-oriented, manageable events */
    IAPOnSendComplete on_send_complete;
    /* for user-oriented, unexpectable events */
    struct IAPActiveEvent active_events[2];
    /* _iap_feed_packet */
    int32_t handling_trans_id;
    /* iap.c */
    struct IAPContextButtons context_button_state;
    struct IAPPlatformArtwork    artwork;
    size_t                       artwork_cursor;
    uint16_t                     trans_id;
    uint16_t                     artwork_chunk_index;
    int32_t                      artwork_trans_id;
    uint16_t                     artwork_data_command;
    uint8_t                      artwork_data_lingo;
    uint8_t                      trans_id_support; /* TransIDSupport */
    /* notification.c */
    /* DisplayRemote::SetRemoteEventNotification */
    uint32_t enabled_notifications_3;
    uint32_t notifications_3;
    /* ExtendedInterface::SetPlayStatusChangeNotification */
    uint32_t enabled_notifications_4;
    uint32_t notifications_4;
    /* notification data */
    struct _IAPNotifyState notification_data;
    uint8_t                notification_tick;
    /* _iap_send_hid_reports */
    uint8_t* hid_send_staging_buf;
    IAPBool  send_busy : 1;
    IAPBool  flushing_notifications : 1;
    IAPBool  waiting_for_audio_attrs_ack : 1;

    uint8_t phase; /* IAPPhase */
};
