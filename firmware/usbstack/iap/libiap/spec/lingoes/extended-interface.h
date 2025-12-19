#pragma once
#include <stdint.h>

/* [1] P.398 Table 5-1 Extended Interface lingo command summary */
enum IAPExtendedInterfaceCommandID {
    IAPExtendedInterfaceCommandID_IPodAck                                    = 0x0001, /* 1.00, NoAuth, from acc, ipod-ack.h */
    IAPExtendedInterfaceCommandID_GetCurrentPlayingTrackChapterInfo          = 0x0002, /* 1.06, NoAuth, from acc, no payload */
    IAPExtendedInterfaceCommandID_ReturnCurrentPlayingTrackChapterInfo       = 0x0003, /* 1.06, NoAuth, from dev, current-playing-track-chapter.h */
    IAPExtendedInterfaceCommandID_SetCurrentPlayingTrackChapter              = 0x0004, /* 1.06, NoAuth, from dev, current-playing-track-chapter.h */
    IAPExtendedInterfaceCommandID_GetCurrentPlayingTrackChapterPlayStatus    = 0x0005, /* 1.06, NoAuth, from dev, current-playing-track-chapter.h */
    IAPExtendedInterfaceCommandID_ReturnCurrentPlayingTrackChapterPlayStatus = 0x0006, /* 1.06, NoAuth, from dev, current-playing-track-chapter.h */
    IAPExtendedInterfaceCommandID_GetCurrentPlayingTrackChapterName          = 0x0007, /* 1.06, NoAuth, from dev, current-playing-track-chapter.h */
    IAPExtendedInterfaceCommandID_ReturnCurrentPlayingTrackChapterName       = 0x0008, /* 1.06, NoAuth, from dev, current-playing-track-chapter.h */
    IAPExtendedInterfaceCommandID_GetAudiobookSpeed                          = 0x0009, /* 1.06, NoAuth, from acc, no payload */
    IAPExtendedInterfaceCommandID_RetAudiobookSpeed                          = 0x000A, /* 1.06, NoAuth, from dev, audiobook-speed.h */
    IAPExtendedInterfaceCommandID_SetAudiobookSpeed                          = 0x000B, /* 1.06, NoAuth, from acc, audiobook-speed.h */
    IAPExtendedInterfaceCommandID_GetIndexedPlayingTrackInfo                 = 0x000C, /* 1.08, NoAuth, from acc, indexed-playing-track-info.h */
    IAPExtendedInterfaceCommandID_ReturnIndexedPlayingTrackInfo              = 0x000D, /* 1.08, NoAuth, from dev, indexed-playing-track-info.h */
    IAPExtendedInterfaceCommandID_GetArtworkFormats                          = 0x000E, /* 1.10, NoAuth, from acc, no payload */
    IAPExtendedInterfaceCommandID_RetArtworkFormats                          = 0x000F, /* 1.10, NoAuth, from dev, display-remote/artwork-formats.h*/
    IAPExtendedInterfaceCommandID_GetTrackArtworkData                        = 0x0010, /* 1.10, NoAuth, from acc, display-remote/track-artwork-data.h*/
    IAPExtendedInterfaceCommandID_RetTrackArtworkData                        = 0x0011, /* 1.10, NoAuth, from dev, display-remote/track-artwork-data.h*/
    IAPExtendedInterfaceCommandID_RequestProtocolVersion                     = 0x0012, /* deprecated */
    IAPExtendedInterfaceCommandID_ReturnProtocolVersion                      = 0x0013, /* deprecated */
    IAPExtendedInterfaceCommandID_RequestIPodName                            = 0x0014, /* deprecated */
    IAPExtendedInterfaceCommandID_ReturnIPodName                             = 0x0015, /* deprecated */
    IAPExtendedInterfaceCommandID_ResetDBSelection                           = 0x0016, /* 1.00, NoAuth, from acc, no payload */
    IAPExtendedInterfaceCommandID_SelectDBRecord                             = 0x0017, /* 1.00, NoAuth, from dev, database.h*/
    IAPExtendedInterfaceCommandID_GetNumberCategorizedDBRecords              = 0x0018, /* 1.00, NoAuth, from acc, database.h */
    IAPExtendedInterfaceCommandID_ReturnNumberCategorizedDBRecords           = 0x0019, /* 1.00, NoAuth, from dev, database.h */
    IAPExtendedInterfaceCommandID_RetrieveCategorizedDatabaseRecords         = 0x001A, /* 1.00, NoAuth, from acc, database.h */
    IAPExtendedInterfaceCommandID_ReturnCategorizedDatabaseRecords           = 0x001B, /* 1.00, NoAuth, from acc, database.h */
    IAPExtendedInterfaceCommandID_GetPlayStatus                              = 0x001C, /* 1.00, NoAuth, no payload */
    IAPExtendedInterfaceCommandID_ReturnPlayStatus                           = 0x001D, /* 1.00, NoAuth, play-status.h */
    IAPExtendedInterfaceCommandID_GetCurrentPlayingTrackIndex                = 0x001E, /* 1.00, NoAuth, from acc, no payload */
    IAPExtendedInterfaceCommandID_ReturnCurrentPlayingTrackIndex             = 0x001F, /* 1.00, NoAuth, from dev, current-playing-index.h */
    IAPExtendedInterfaceCommandID_GetIndexedPlayingTrackTitle                = 0x0020, /* 1.00, NoAuth, from acc, indexed-playing-track-string.h */
    IAPExtendedInterfaceCommandID_ReturnIndexedPlayingTrackTitle             = 0x0021, /* 1.00, NoAuth, from dev, indexed-playing-track-string.h */
    IAPExtendedInterfaceCommandID_GetIndexedPlayingTrackArtistName           = 0x0022, /* 1.00, NoAuth, from acc, indexed-playing-track-string.h */
    IAPExtendedInterfaceCommandID_ReturnIndexedPlayingTrackArtistName        = 0x0023, /* 1.00, NoAuth, from dev, indexed-playing-track-string.h */
    IAPExtendedInterfaceCommandID_GetIndexedPlayingTrackAlbumName            = 0x0024, /* 1.00, NoAuth, from acc, indexed-playing-track-string.h */
    IAPExtendedInterfaceCommandID_ReturnIndexedPlayingTrackAlbumName         = 0x0025, /* 1.00, NoAuth, from dev, indexed-playing-track-string.h */
    IAPExtendedInterfaceCommandID_SetPlayStatusChangeNotification            = 0x0026, /* 1.00, NoAuth, from acc, play-status-change-notification.h */
    IAPExtendedInterfaceCommandID_PlayStatusChangeNotification               = 0x0027, /* 1.00, NoAuth, from dev, play-status-change-notification.h */
    IAPExtendedInterfaceCommandID_PlayCurrentSelection                       = 0x0028, /* 1.00, NoAuth, from acc, play-current-selection.h */
    IAPExtendedInterfaceCommandID_PlayControl                                = 0x0029, /* 1.00, NoAuth, from acc, play-control.h */
    IAPExtendedInterfaceCommandID_GetTrackArtworkTimes                       = 0x002A, /* 1.10, NoAuth, from acc, display-remote/track-artwork-times.h */
    IAPExtendedInterfaceCommandID_RetTrackArtworkTimes                       = 0x002B, /* 1.10, NoAuth, from dev, display-remote/track-artwork-times.h */
    IAPExtendedInterfaceCommandID_GetShuffle                                 = 0x002C, /* 1.00, NoAuth, from acc, no payload */
    IAPExtendedInterfaceCommandID_ReturnShuffle                              = 0x002D, /* 1.00, NoAuth, from dev, shuffle.h */
    IAPExtendedInterfaceCommandID_SetShuffle                                 = 0x002E, /* 1.00, NoAuth, from acc, shuffle.h */
    IAPExtendedInterfaceCommandID_GetRepeat                                  = 0x002F, /* 1.00, NoAuth, from acc, no payload */
    IAPExtendedInterfaceCommandID_ReturnRepeat                               = 0x0030, /* 1.00, NoAuth, from dev, repeat.h */
    IAPExtendedInterfaceCommandID_SetRepeat                                  = 0x0031, /* 1.00, NoAuth, from acc, repeat.h */
    IAPExtendedInterfaceCommandID_SetDisplayImage                            = 0x0032, /* 1.01, NoAuth, from acc, set-display-image.h */
    IAPExtendedInterfaceCommandID_GetMonoDisplayImageLimits                  = 0x0033, /* 1.01, NoAuth, from acc, no payload */
    IAPExtendedInterfaceCommandID_ReturnMonoDisplayImageLimits               = 0x0034, /* 1.01, NoAuth, from dev, mono-display-image-limits.h */
    IAPExtendedInterfaceCommandID_GetNumPlayingTracks                        = 0x0035, /* 1.01, NoAuth, from acc, no payload */
    IAPExtendedInterfaceCommandID_ReturnNumPlayingTracks                     = 0x0036, /* 1.01, NoAuth, from dev, display-remote/num-playing-tracks.h */
    IAPExtendedInterfaceCommandID_SetCurrentPlayingTrack                     = 0x0037, /* 1.01, NoAuth, from acc, display-remote/set-current-playing-track.h */
    IAPExtendedInterfaceCommandID_SelectSortDBRecord                         = 0x0038, /* deprecated */
    IAPExtendedInterfaceCommandID_GetColorDisplayImageLimits                 = 0x0039, /* 1.09, NoAuth, from acc, no payload */
    IAPExtendedInterfaceCommandID_ReturnColorDisplayImageLimits              = 0x003A, /* 1.09, NoAuth, from dev, color-display-image-limits.h */
    IAPExtendedInterfaceCommandID_ResetDBSelectionHierarchy                  = 0x003B, /* 1.11, Auth, from acc, db-selection-hierarchy.h */
    IAPExtendedInterfaceCommandID_GetDBITunesInfo                            = 0x003C, /* 1.13, Auth, from acc, db-itunes-info.h */
    IAPExtendedInterfaceCommandID_RetDBITunesInfo                            = 0x003D, /* 1.13, Auth, from dev, db-itunes-info.h */
    IAPExtendedInterfaceCommandID_GetUIDTrackInfo                            = 0x003E, /* 1.13, Auth, from acc, track-info.h */
    IAPExtendedInterfaceCommandID_RetUIDTrackInfo                            = 0x003F, /* 1.13, Auth, from dev, track-info.h */
    IAPExtendedInterfaceCommandID_GetDBTrackInfo                             = 0x0040, /* 1.13, Auth, from acc, track-info.h */
    IAPExtendedInterfaceCommandID_RetDBTrackInfo                             = 0x0041, /* 1.13, Auth, from dev, track-info.h */
    IAPExtendedInterfaceCommandID_GetPBTrackInfo                             = 0x0042, /* 1.13, Auth, from acc, track-info.h */
    IAPExtendedInterfaceCommandID_RetPBTrackInfo                             = 0x0043, /* 1.13, Auth, from dev, track-info.h */
    IAPExtendedInterfaceCommandID_CreateGeniusPlaylist                       = 0x0044, /* 1.13, Auth, from acc, genius-playlist.h */
    IAPExtendedInterfaceCommandID_RefreshGeniusPlaylist                      = 0x0045, /* 1.13, Auth, from acc, genius-playlist.h */
    IAPExtendedInterfaceCommandID_IsGeniusAvailableForTrack                  = 0x0047, /* 1.13, Auth, from acc, genius-playlist.h */
    IAPExtendedInterfaceCommandID_GetPlaylistInfo                            = 0x0048, /* 1.13, Auth, from acc, playlist-info.h */
    IAPExtendedInterfaceCommandID_RetPlaylistInfo                            = 0x0049, /* 1.13, Auth, from dev, playlist-info.h */
    IAPExtendedInterfaceCommandID_PrepareUIDList                             = 0x004A, /* 1.14, Auth, from dev, uid-list.h */
    IAPExtendedInterfaceCommandID_PlayPreparedUIDList                        = 0x004B, /* 1.14, Auth, from dev, uid-list.h */
    IAPExtendedInterfaceCommandID_GetArtworkTimes                            = 0x004C, /* 1.14, Auth, from acc, artwork-times.h */
    IAPExtendedInterfaceCommandID_RetArtworkTimes                            = 0x004D, /* 1.14, Auth, from dev, artwork-times.h */
    IAPExtendedInterfaceCommandID_GetArtworkData                             = 0x004E, /* 1.14, Auth, from acc, artwork-data.h */
    IAPExtendedInterfaceCommandID_RetArtworkData                             = 0x004F, /* 1.14, Auth, from dev, artwork-data.h */
};

#include "extended-interface/artwork-data.h"
#include "extended-interface/artwork-times.h"
#include "extended-interface/audiobook-speed.h"
#include "extended-interface/color-display-image-limits.h"
#include "extended-interface/current-playing-track-chapter.h"
#include "extended-interface/current-playing-track-index.h"
#include "extended-interface/database.h"
#include "extended-interface/db-itunes-info.h"
#include "extended-interface/db-selection-hierarchy.h"
#include "extended-interface/genius-playlist.h"
#include "extended-interface/indexed-playing-track-info.h"
#include "extended-interface/indexed-playing-track-string.h"
#include "extended-interface/ipod-ack.h"
#include "extended-interface/mono-display-image-limits.h"
#include "extended-interface/play-control.h"
#include "extended-interface/play-current-selection.h"
#include "extended-interface/play-status-change-notification.h"
#include "extended-interface/play-status.h"
#include "extended-interface/playlist-info.h"
#include "extended-interface/repeat.h"
#include "extended-interface/set-display-image.h"
#include "extended-interface/shuffle.h"
#include "extended-interface/track-info.h"
#include "extended-interface/uid-list.h"
