#pragma once
#include <stdint.h>

enum IAPIPodNotificationType {
    IAPIPodNotificationType_FlowControl         = 2,
    IAPIPodNotificationType_RadioTagging        = 3,
    IAPIPodNotificationType_Camera              = 4,
    IAPIPodNotificationType_ChargingInfo        = 5,
    IAPIPodNotificationType_DatabaseChanged     = 9,
    IAPIPodNotificationType_AppBundleName       = 10,
    IAPIPodNotificationType_SessionSpaceAvail   = 11,
    IAPIPodNotificationType_CommandComplete     = 13,
    IAPIPodNotificationType_IPodOutMode         = 15,
    IAPIPodNotificationType_BluetoothConnection = 17,
    IAPIPodNotificationType_AppDisplayName      = 19,
    IAPIPodNotificationType_AssistiveTouch      = 20,
};

struct IAPIPodNotificationPayload {
    uint8_t type; /* IAPIPodNotificationType */
    uint8_t data[];
} __attribute__((packed));

struct IAPIPodNotificationFlowControlPayload {
    uint8_t  type; /* = IAPIPodNotificationType_FlowControl */
    uint32_t wait_time_ms;
    uint16_t overflow_transaction_id;
} __attribute__((packed));

enum IAPIPodNotificationRadioTaggingStatus {
    IAPIPodNotificationRadioTaggingStatus_Success      = 0x00,
    IAPIPodNotificationRadioTaggingStatus_Failed       = 0x01,
    IAPIPodNotificationRadioTaggingStatus_InfoAvail    = 0x02,
    IAPIPodNotificationRadioTaggingStatus_InfoNotAvail = 0x03,
};

struct IAPIPodNotificationRadioTaggingPayload {
    uint8_t type;   /* = IAPIPodNotificationType_RadioTagging */
    uint8_t status; /* IAPIPodNotificationRadioTaggingStatus */
} __attribute__((packed));

enum IAPIPodNotificationCameraStatus {
    IAPIPodNotificationCameraStatus_AppOff    = 0x00,
    IAPIPodNotificationCameraStatus_Preview   = 0x03,
    IAPIPodNotificationCameraStatus_Recording = 0x04,
};

struct IAPIPodNotificationCameraPayload {
    uint8_t type;   /* = IAPIPodNotificationType_Camera */
    uint8_t status; /* IAPIPodNotificationCameraStatus */
} __attribute__((packed));

enum IAPIPodNotificationChargingInfoType {
    IAPIPodNotificationChargingInfoType_AvailCurrent = 0x00,
};

struct IAPIPodNotificationChargingInfoPayload {
    uint8_t  type;      /* = IAPIPodNotificationType_ChargingInfo */
    uint8_t  info_type; /* IAPIPodNotificationChargingInfoType */
    uint16_t info_value;
} __attribute__((packed));

struct IAPIPodNotificationDatabaseChangedPayload {
    uint8_t type; /* = IAPIPodNotificationType_DatabaseChanged */
} __attribute__((packed));

struct IAPIPodNotificationAppBundleNamePayload {
    uint8_t type; /* = IAPIPodNotificationType_AppBundleName */
    char    name[];
} __attribute__((packed));

struct IAPIPodNotificationSessionSpaceAvailPayload {
    uint8_t  type; /* = IAPIPodNotificationType_SessionSpaceAvail */
    uint16_t session_id;
} __attribute__((packed));

enum IAPIPodNotificationCommandCompleteStatus {
    IAPIPodNotificationCommandCompleteStatus_Success   = 0x00,
    IAPIPodNotificationCommandCompleteStatus_Failed    = 0x01,
    IAPIPodNotificationCommandCompleteStatus_Cancelled = 0x02,
};

struct IAPIPodNotificationCommandCompletePayload {
    uint8_t  type; /* = IAPIPodNotificationType_CommandComplete */
    uint8_t  lingo_id;
    uint16_t command_id;
    uint8_t  status; /* IAPIPodNotificationCommandCompleteStatus */
} __attribute__((packed));

struct IAPIPodNotificationIPodOutModePayload {
    uint8_t type; /* = IAPIPodNotificationType_IPodOutMode */
    uint8_t is_active;
} __attribute__((packed));

enum IAPIPodNotificationBluetoothConnectionStatusBits {
    IAPIPodNotificationBluetoothConnectionStatusBits_HFP     = 1 << 0,
    IAPIPodNotificationBluetoothConnectionStatusBits_PBAP    = 1 << 1,
    IAPIPodNotificationBluetoothConnectionStatusBits_AVRCP   = 1 << 3,
    IAPIPodNotificationBluetoothConnectionStatusBits_A2DP    = 1 << 4,
    IAPIPodNotificationBluetoothConnectionStatusBits_HID     = 1 << 5,
    IAPIPodNotificationBluetoothConnectionStatusBits_IAP     = 1 << 7,
    IAPIPodNotificationBluetoothConnectionStatusBits_PAN_NAP = 1 << 8,
    IAPIPodNotificationBluetoothConnectionStatusBits_PAN_U   = 1 << 12,
};

struct IAPIPodNotificationBluetoothConnectionPayload {
    uint8_t  type; /* = IAPIPodNotificationType_BluetoothConnection */
    uint8_t  mac_addr[6];
    uint64_t status; /* IAPIPodNotificationBluetoothConnectionStatusBits */
} __attribute__((packed));

struct IAPIPodNotificationAppDisplayNamePayload {
    uint8_t type; /* = IAPIPodNotificationType_AppDisplayName */
    char    name[];
} __attribute__((packed));

struct IAPIPodNotificationAssistiveTouchPayload {
    uint8_t type; /* = IAPIPodNotificationType_AssistiveTouch */
    uint8_t is_on;
} __attribute__((packed));
