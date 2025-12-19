#pragma once
#include <stdint.h>

/* [1] P.249 Table 4-48 Display Remote lingo command summary */
enum IAPDisplayRemoteCommandID {
    IAPDisplayRemoteCommandID_IPodAck                    = 0x00, /* from acc, general/ipod-ack.h */
    IAPDisplayRemoteCommandID_GetCurrentEQProfileIndex   = 0x01, /* from acc, no payload */
    IAPDisplayRemoteCommandID_RetCurrentEQProfileIndex   = 0x02, /* from dev, current-eq-profile-index.h */
    IAPDisplayRemoteCommandID_SetCurrentEQProfileIndex   = 0x03, /* from acc, current-eq-profile-index.h */
    IAPDisplayRemoteCommandID_GetNumEQProfiles           = 0x04, /* from acc, no payload */
    IAPDisplayRemoteCommandID_RetNumEQProfiles           = 0x05, /* from dev, num-eq-profiles.h */
    IAPDisplayRemoteCommandID_GetIndexedEQProfileName    = 0x06, /* from acc, indexed-eq-profile-name.h */
    IAPDisplayRemoteCommandID_RetIndexedEQProfileName    = 0x07, /* from dev, indexed-eq-profile-name.h */
    IAPDisplayRemoteCommandID_SetRemoteEventNotification = 0x08, /* from acc, ipod-state.h */
    IAPDisplayRemoteCommandID_RemoteEventNotification    = 0x09, /* from dev, ipod-state.h */
    IAPDisplayRemoteCommandID_GetRemoteEventStatus       = 0x0A, /* from acc, no payload */
    IAPDisplayRemoteCommandID_RetRemoteEventStatus       = 0x0B, /* from dev, remote-event-status.h */
    IAPDisplayRemoteCommandID_GetIPodStateInfo           = 0x0C, /* from acc, ipod-state.h */
    IAPDisplayRemoteCommandID_RetIPodStateInfo           = 0x0D, /* from dev, ipod-state.h */
    IAPDisplayRemoteCommandID_SetIPodStateInfo           = 0x0E, /* from acc, ipod-state.h */
    IAPDisplayRemoteCommandID_GetPlayStatus              = 0x0F, /* from acc, no payload */
    IAPDisplayRemoteCommandID_RetPlayStatus              = 0x10, /* from dev, play-status.h */
    IAPDisplayRemoteCommandID_SetCurrentPlayingTrack     = 0x11, /* from acc, set-current-playing-track.h */
    IAPDisplayRemoteCommandID_GetIndexedPlayingTrackInfo = 0x12, /* from acc, indexed-playing-track-info.h */
    IAPDisplayRemoteCommandID_RetIndexedPlayingTrackInfo = 0x13, /* from dev, indexed-playing-track-info.h */
    IAPDisplayRemoteCommandID_GetNumPlayingTracks        = 0x14, /* from acc, no payload */
    IAPDisplayRemoteCommandID_RetNumPlayingTracks        = 0x15, /* from dev, num-playing-tracks.h */
    IAPDisplayRemoteCommandID_GetArtworkFormats          = 0x16, /* from acc, no payload */
    IAPDisplayRemoteCommandID_RetArtworkFormats          = 0x17, /* from dev, artwork-formats.h */
    IAPDisplayRemoteCommandID_GetTrackArtworkData        = 0x18, /* from acc, track-artwork-data.h */
    IAPDisplayRemoteCommandID_RetTrackArtworkData        = 0x19, /* from dev, track-artwork-data.h */
    IAPDisplayRemoteCommandID_GetPowerBatteryState       = 0x1A, /* from acc, no payload */
    IAPDisplayRemoteCommandID_RetPowerBatteryState       = 0x1B, /* from dev, power-battery-state.h */
    IAPDisplayRemoteCommandID_GetSoundCheckState         = 0x1C, /* from acc, no payload */
    IAPDisplayRemoteCommandID_RetSoundCheckState         = 0x1D, /* from dev, sound-check-status.h */
    IAPDisplayRemoteCommandID_SetSoundCheckState         = 0x1E, /* from acc, sound-check-status.h */
    IAPDisplayRemoteCommandID_GetTrackArtworkTimes       = 0x1F, /* from acc, track-artwork-times.h */
    IAPDisplayRemoteCommandID_RetTrackArtworkTimes       = 0x20, /* from dev, track-artwork-times.h */
    IAPDisplayRemoteCommandID_CreateGeniusPlaylist       = 0x21, /* from acc, genius_playlist.h */
    IAPDisplayRemoteCommandID_IsGeniusAvailableForTrack  = 0x22, /* from acc, genius_playlist.h */
};

#include "display-remote/artwork-formats.h"
#include "display-remote/current-eq-profile-index.h"
#include "display-remote/genius-playlist.h"
#include "display-remote/indexed-eq-profile-name.h"
#include "display-remote/indexed-playing-track-info.h"
#include "display-remote/ipod-state.h"
#include "display-remote/num-eq-profiles.h"
#include "display-remote/num-playing-tracks.h"
#include "display-remote/play-status.h"
#include "display-remote/power-battery-state.h"
#include "display-remote/remote-event-status.h"
#include "display-remote/set-current-playing-track.h"
#include "display-remote/track-artwork-data.h"
#include "display-remote/track-artwork-times.h"
