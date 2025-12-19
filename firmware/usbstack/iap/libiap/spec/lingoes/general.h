#pragma once
#include <stdint.h>

/* [1] P.119 Table 3-1 General lingo commands */
enum IAPGeneralCommandID {
    IAPGeneralCommandID_RequestIdentify                     = 0x00, /* 0.00, from dev, no payload */
    IAPGeneralCommandID_Identify                            = 0x01, /* 0.00, from acc, identify.h, deprecated */
    IAPGeneralCommandID_IPodAck                             = 0x02, /* 1.00, from dev, ipod-ack.h */
    IAPGeneralCommandID_RequestExtendedInterfaceMode        = 0x03, /* 1.00, from acc, no payload */
    IAPGeneralCommandID_ReturnExtendedInterfaceMode         = 0x04, /* 1.00, from dev, extended-interface-mode.h */
    IAPGeneralCommandID_EnterExtendedInterfaceMode          = 0x05, /* 1.00, from acc, no payload, deprecated */
    IAPGeneralCommandID_ExitExtendedInterfaceMode           = 0x06, /* 1.00, from acc, no payload */
    IAPGeneralCommandID_RequestIPodName                     = 0x07, /* 1.00, from acc, no payload */
    IAPGeneralCommandID_ReturnIPodName                      = 0x08, /* 1.00, from dev, ipod-name.h */
    IAPGeneralCommandID_RequestIPodSoftwareVersion          = 0x09, /* 1.00, from acc, no payload */
    IAPGeneralCommandID_ReturnIPodSoftwareVersion           = 0x0A, /* 1.00, from dev, ipod-software-version.h */
    IAPGeneralCommandID_RequestIPodSerialNum                = 0x0B, /* 1.00, from acc, no payload */
    IAPGeneralCommandID_ReturnIPodSerialNum                 = 0x0C, /* 1.00, from dev, ipod-serial-num.h */
    IAPGeneralCommandID_RequestIPodModelNum                 = 0x0D, /* 1.00, from acc, deprecated */
    IAPGeneralCommandID_ReturnIPodModelNum                  = 0x0E, /* 1.00, from dev, deprecated */
    IAPGeneralCommandID_RequestLingoProtocolVersion         = 0x0F, /* 1.00, from acc, lingo-protocol-version.h */
    IAPGeneralCommandID_ReturnLingoProtocolVersion          = 0x10, /* 1.00, from dev, lingo-protocol-version.h */
    IAPGeneralCommandID_RequestTransportMaxPayloadSize      = 0x11, /* 1.09, from acc, no payload  */
    IAPGeneralCommandID_ReturnTransportMaxPayloadSize       = 0x12, /* 1.09, from dev, transport-max-payload-size.h */
    IAPGeneralCommandID_IdentifyDeviceLingoes               = 0x13, /* 1.01, from acc, identify-device-lingoes.h */
    IAPGeneralCommandID_GetAccessoryAuthenticationInfo      = 0x14, /* 1.01, from dev, no payload */
    IAPGeneralCommandID_RetAccessoryAuthenticationInfo      = 0x15, /* 1.01, from acc, acc-auth-info.h */
    IAPGeneralCommandID_AckAccessoryAuthenticationInfo      = 0x16, /* 1.01, from dev, acc-auth-info.h */
    IAPGeneralCommandID_GetAccessoryAuthenticationSignature = 0x17, /* 1.01, from dev, acc-auth-sig.h */
    IAPGeneralCommandID_RetAccessoryAuthenticationSignature = 0x18, /* 1.01, from acc, acc-auth-sig.h */
    IAPGeneralCommandID_AckAccessoryAuthenticationStatus    = 0x19, /* 1.01, from dev, acc-auth-sig.h*/
    IAPGeneralCommandID_GetIPodAuthenticationInfo           = 0x1A, /* 1.01, from acc, no payload */
    IAPGeneralCommandID_RetIPodAuthenticationInfo           = 0x1B, /* 1.01, from dev, ipod-auth-info.h */
    IAPGeneralCommandID_AckIPodAuthenticationInfo           = 0x1C, /* 1.01, from acc, ipod-auth-info.h */
    IAPGeneralCommandID_GetIPodAuthenticationSignature      = 0x1D, /* 1.01, from acc, ipod-auth-sig.h */
    IAPGeneralCommandID_RetIPodAuthenticationSignature      = 0x1E, /* 1.01, from dev, ipod-auth-sig.h */
    IAPGeneralCommandID_AckIPodAuthenticationStatus         = 0x1F, /* 1.01, from acc, ipod-auth-sig.h */
    IAPGeneralCommandID_NotifyIPodStateChange               = 0x23, /* 1.02, from dev, notify-ipod-state-change.h */
    IAPGeneralCommandID_GetIPodOptions                      = 0x24, /* 1.05, from acc, no payload */
    IAPGeneralCommandID_RetIPodOptions                      = 0x25, /* 1.05, from dev, ipod-options.h */
    IAPGeneralCommandID_GetAccessoryInfo                    = 0x27, /* 1.04, from dev, acc-info.h */
    IAPGeneralCommandID_RetAccessoryInfo                    = 0x28, /* 1.04, from acc, acc-info.h */
    IAPGeneralCommandID_GetIPodPreferences                  = 0x29, /* 1.05, from acc, ipod-preferences.h */
    IAPGeneralCommandID_RetIPodPreferences                  = 0x2A, /* 1.05, from dev, ipod-preferences.h */
    IAPGeneralCommandID_SetIPodPreferences                  = 0x2B, /* 1.05, from acc, ipod-preferences.h */
    IAPGeneralCommandID_GetUIMode                           = 0x35, /* 1.09, from acc, no payload */
    IAPGeneralCommandID_RetUIMode                           = 0x36, /* 1.09, from dev, ui-mode.h */
    IAPGeneralCommandID_SetUIMode                           = 0x37, /* 1.09, from acc, ui-mode.h */
    IAPGeneralCommandID_StartIDPS                           = 0x38, /* 1.09, from acc, no payload */
    IAPGeneralCommandID_SetFIDTokenValues                   = 0x39, /* 1.09, from acc, fid-token-values.h */
    IAPGeneralCommandID_AckFIDTokenValues                   = 0x3A, /* 1.09, from dev, fid-token-values.h */
    IAPGeneralCommandID_EndIDPS                             = 0x3B, /* 1.09, from acc, end-idps.h */
    IAPGeneralCommandID_IDPSStatus                          = 0x3C, /* 1.09, from dev, end-idps.h */
    IAPGeneralCommandID_OpenDataSessionForProtocol          = 0x3F, /* 1.09, from dev, data-session.h */
    IAPGeneralCommandID_CloseDataSession                    = 0x40, /* 1.09, from dev, data-session.h */
    IAPGeneralCommandID_AccessoryAck                        = 0x41, /* 1.09, from acc, acc-ack.h */
    IAPGeneralCommandID_AccessoryDataTransfer               = 0x42, /* 1.09, from acc, data-transfer.h */
    IAPGeneralCommandID_IPodDataTransfer                    = 0x43, /* 1.09, from dev, data-transfer.h */
    IAPGeneralCommandID_SetAccessoryStatusNotification      = 0x46, /* 1.09, from dev, acc-status-notification.h */
    IAPGeneralCommandID_RetAccessoryStatusNotification      = 0x47, /* 1.09, from acc, acc-status-notification.h */
    IAPGeneralCommandID_AccessoryStatusNotification         = 0x48, /* 1.09, from acc, acc-status-notification.h */
    IAPGeneralCommandID_SetEventNotification                = 0x49, /* 1.09, from acc, event-notification.h */
    IAPGeneralCommandID_IPodNotification                    = 0x4A, /* 1.09, from dev, ipod-notifiction.h */
    IAPGeneralCommandID_GetIPodOptionsForLingo              = 0x4B, /* 1.09, from acc, ipod-options-for-lingo.h */
    IAPGeneralCommandID_RetIPodOptionsForLingo              = 0x4C, /* 1.09, from dev, ipod-options-for-lingo.h */
    IAPGeneralCommandID_GetEventNotification                = 0x4D, /* 1.09, from acc, no payload */
    IAPGeneralCommandID_RetEventNotification                = 0x4E, /* 1.09, from dev, event-notification.h */
    IAPGeneralCommandID_GetSupportedEventNotification       = 0x4F, /* 1.09, from acc, no payload */
    IAPGeneralCommandID_CancelCommand                       = 0x50, /* 1.09, from acc, cancel-command.h */
    IAPGeneralCommandID_RetSupportedEventNotification       = 0x51, /* 1.09, from dev, event-notification.h */
    IAPGeneralCommandID_SetAvailableCurrent                 = 0x54, /* 1.09, from acc, set-available-current.h */
    IAPGeneralCommandID_SetInternalBatteryChargingState     = 0x56, /* 1.09, from acc, set-internal-battery-charging-state.h */
    IAPGeneralCommandID_RequestApplicationLaunch            = 0x64, /* 1.09, from acc, request-application-launch.h */
    IAPGeneralCommandID_GetNowPlayingApplicationBundleName  = 0x65, /* 1.09, from acc, no payload */
    IAPGeneralCommandID_RetNowPlayingApplicationBundleName  = 0x66, /* 1.09, from dev, now-playing-app-bundle-name.h */
    IAPGeneralCommandID_GetLocalizationInfo                 = 0x67, /* 1.09, from acc, localization-info.h */
    IAPGeneralCommandID_RetLocalizationInfo                 = 0x68, /* 1.09, from dev, localization-info.h */
    IAPGeneralCommandID_RequestWiFiConnectionInfo           = 0x69, /* 1.09, from acc, no payload */
    IAPGeneralCommandID_WiFiConnectionInfo                  = 0x6A, /* 1.09, from dev, wifi-connection-info.h */
};

#include "general/acc-ack.h"
#include "general/acc-auth-info.h"
#include "general/acc-auth-sig.h"
#include "general/acc-info.h"
#include "general/acc-status-notification.h"
#include "general/cancel-command.h"
#include "general/data-session.h"
#include "general/data-transfer.h"
#include "general/end-idps.h"
#include "general/event-notification.h"
#include "general/extended-interface-mode.h"
#include "general/fid-token-values.h"
#include "general/identify-device-lingoes.h"
#include "general/identify.h"
#include "general/ipod-ack.h"
#include "general/ipod-auth-info.h"
#include "general/ipod-auth-sig.h"
#include "general/ipod-name.h"
#include "general/ipod-notification.h"
#include "general/ipod-options-for-lingo.h"
#include "general/ipod-options.h"
#include "general/ipod-preferences.h"
#include "general/ipod-software-version.h"
#include "general/lingo-protocol-version.h"
#include "general/localization-info.h"
#include "general/now-playing-app-bundle-name.h"
#include "general/request-application-launch.h"
#include "general/set-available-current.h"
#include "general/transport-max-payload-size.h"
#include "general/ui-mode.h"
#include "general/wifi-connection-info.h"
