#pragma once
#include <stdint.h>

enum IAPDatabaseType {
    IAPDatabaseType_TopLevel       = 0x00, /* 1.14 */
    IAPDatabaseType_Playlist       = 0x01, /* 1.00 */
    IAPDatabaseType_Artist         = 0x02, /* 1.00 */
    IAPDatabaseType_Album          = 0x03, /* 1.00 */
    IAPDatabaseType_Genre          = 0x04, /* 1.00 */
    IAPDatabaseType_Track          = 0x05, /* 1.00 */
    IAPDatabaseType_Composer       = 0x06, /* 1.00 */
    IAPDatabaseType_Audiobook      = 0x07, /* 1.06 */
    IAPDatabaseType_Podcast        = 0x08, /* 1.08 */
    IAPDatabaseType_NestedPlaylist = 0x09, /* 1.13 */
    IAPDatabaseType_GeniusMixes    = 0x0A, /* 1.14 */
    IAPDatabaseType_ITunesU        = 0x0B, /* 1.14 */

};

struct IAPSelectDBRecord {
    uint8_t  type; /* IAPDatabaseType */
    uint32_t index;
} __attribute__((packed));

struct IAPGetNumberCategorizedDBRecordsPayload {
    uint8_t type; /* IAPDatabaseType */
} __attribute__((packed));

struct IAPReturnNumberCategorizedDBRecordsPayload {
    uint32_t count;
} __attribute__((packed));

struct IAPRetrieveCategorizedDatabaseRecordsPayload {
    uint8_t  type; /* IAPDatabaseType */
    uint32_t index;
    uint32_t count;
} __attribute__((packed));

struct IAPReturnCategorizedDatabaseRecordsPayload {
    uint32_t index;
    char     record[];
} __attribute__((packed));
