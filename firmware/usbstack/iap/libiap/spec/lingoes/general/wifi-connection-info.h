#pragma once
#include <stdint.h>

enum IAPWiFiConnectionInfoStatus {
    IAPWiFiConnectionInfoStatus_Success     = 0x00,
    IAPWiFiConnectionInfoStatus_Unavailable = 0x01,
    IAPWiFiConnectionInfoStatus_Declined    = 0x02,
    IAPWiFiConnectionInfoStatus_Failed      = 0x03,
};

enum IAPWiFiConnectionInfoSecurityType {
    IAPWiFiConnectionInfoSecurityType_Unsecured    = 0x00,
    IAPWiFiConnectionInfoSecurityType_WEP          = 0x01,
    IAPWiFiConnectionInfoSecurityType_WPA          = 0x02,
    IAPWiFiConnectionInfoSecurityType_WPA2         = 0x03,
    IAPWiFiConnectionInfoSecurityType_MixedWPAWPA2 = 0x04,
};

struct IAPWiFiConnectionInfoPayload {
    uint8_t status;        /* IAPWiFiConnectionInfoStatus */
    uint8_t security_type; /* IAPWiFiConnectionInfoSecurityType */
    char    ssid[32];
    char    passphrase[];
} __attribute__((packed));
