#include <inttypes.h>
#include <stdint.h>

#include "endian.h"
#include "macros.h"
#include "span.h"
#include "spec/iap.h"

#define entry(lingo, command) [IAP##lingo##CommandID_##command] = #command

static const char* strs_general[] = {
    entry(General, RequestIdentify),
    entry(General, Identify),
    entry(General, IPodAck),
    entry(General, RequestExtendedInterfaceMode),
    entry(General, ReturnExtendedInterfaceMode),
    entry(General, EnterExtendedInterfaceMode),
    entry(General, ExitExtendedInterfaceMode),
    entry(General, RequestIPodName),
    entry(General, ReturnIPodName),
    entry(General, RequestIPodSoftwareVersion),
    entry(General, ReturnIPodSoftwareVersion),
    entry(General, RequestIPodSerialNum),
    entry(General, ReturnIPodSerialNum),
    entry(General, RequestIPodModelNum),
    entry(General, ReturnIPodModelNum),
    entry(General, RequestLingoProtocolVersion),
    entry(General, ReturnLingoProtocolVersion),
    entry(General, RequestTransportMaxPayloadSize),
    entry(General, ReturnTransportMaxPayloadSize),
    entry(General, IdentifyDeviceLingoes),
    entry(General, GetAccessoryAuthenticationInfo),
    entry(General, RetAccessoryAuthenticationInfo),
    entry(General, AckAccessoryAuthenticationInfo),
    entry(General, GetAccessoryAuthenticationSignature),
    entry(General, RetAccessoryAuthenticationSignature),
    entry(General, AckAccessoryAuthenticationStatus),
    entry(General, GetIPodAuthenticationInfo),
    entry(General, RetIPodAuthenticationInfo),
    entry(General, AckIPodAuthenticationInfo),
    entry(General, GetIPodAuthenticationSignature),
    entry(General, RetIPodAuthenticationSignature),
    entry(General, AckIPodAuthenticationStatus),
    entry(General, NotifyIPodStateChange),
    entry(General, GetIPodOptions),
    entry(General, RetIPodOptions),
    entry(General, GetAccessoryInfo),
    entry(General, RetAccessoryInfo),
    entry(General, GetIPodPreferences),
    entry(General, RetIPodPreferences),
    entry(General, SetIPodPreferences),
    entry(General, GetUIMode),
    entry(General, RetUIMode),
    entry(General, SetUIMode),
    entry(General, StartIDPS),
    entry(General, SetFIDTokenValues),
    entry(General, AckFIDTokenValues),
    entry(General, EndIDPS),
    entry(General, IDPSStatus),
    entry(General, OpenDataSessionForProtocol),
    entry(General, CloseDataSession),
    entry(General, AccessoryAck),
    entry(General, AccessoryDataTransfer),
    entry(General, IPodDataTransfer),
    entry(General, SetAccessoryStatusNotification),
    entry(General, RetAccessoryStatusNotification),
    entry(General, AccessoryStatusNotification),
    entry(General, SetEventNotification),
    entry(General, IPodNotification),
    entry(General, GetIPodOptionsForLingo),
    entry(General, RetIPodOptionsForLingo),
    entry(General, GetEventNotification),
    entry(General, RetEventNotification),
    entry(General, GetSupportedEventNotification),
    entry(General, CancelCommand),
    entry(General, RetSupportedEventNotification),
    entry(General, SetAvailableCurrent),
    entry(General, SetInternalBatteryChargingState),
    entry(General, RequestApplicationLaunch),
    entry(General, GetNowPlayingApplicationBundleName),
    entry(General, RetNowPlayingApplicationBundleName),
    entry(General, GetLocalizationInfo),
    entry(General, RetLocalizationInfo),
    entry(General, RequestWiFiConnectionInfo),
    entry(General, WiFiConnectionInfo),
};
static const char* strs_simple[] = {
    entry(SimpleRemote, ContextButtonStatus),
    entry(SimpleRemote, IPodAck),
    entry(SimpleRemote, ImageButtonStatus),
    entry(SimpleRemote, VideoButtonStatus),
    entry(SimpleRemote, AudioButtonStatus),
    entry(SimpleRemote, IPodOutButtonStatus),
    entry(SimpleRemote, RotationInputStatus),
    entry(SimpleRemote, RadioButtonStatus),
    entry(SimpleRemote, CameraButtonStatus),
    entry(SimpleRemote, RegisterDescriptor),
    entry(SimpleRemote, IPodHIDReport),
    entry(SimpleRemote, AccessoryHIDReport),
    entry(SimpleRemote, UnregisterDescriptor),
    entry(SimpleRemote, VoiceOverEvent),
    entry(SimpleRemote, GetVoiceOverParameter),
    entry(SimpleRemote, RetVoiceOverParameter),
    entry(SimpleRemote, SetVoiceOverParameter),
    entry(SimpleRemote, GetCurrentVoiceOverItemProperty),
    entry(SimpleRemote, RetCurrentVoiceOverItemProperty),
    entry(SimpleRemote, SetVoiceOverContext),
    entry(SimpleRemote, VoiceOverParameterChanged),
    entry(SimpleRemote, AccessoryAck),
};
static const char* strs_display[] = {
    entry(DisplayRemote, IPodAck),
    entry(DisplayRemote, GetCurrentEQProfileIndex),
    entry(DisplayRemote, RetCurrentEQProfileIndex),
    entry(DisplayRemote, SetCurrentEQProfileIndex),
    entry(DisplayRemote, GetNumEQProfiles),
    entry(DisplayRemote, RetNumEQProfiles),
    entry(DisplayRemote, GetIndexedEQProfileName),
    entry(DisplayRemote, RetIndexedEQProfileName),
    entry(DisplayRemote, SetRemoteEventNotification),
    entry(DisplayRemote, RemoteEventNotification),
    entry(DisplayRemote, GetRemoteEventStatus),
    entry(DisplayRemote, RetRemoteEventStatus),
    entry(DisplayRemote, GetIPodStateInfo),
    entry(DisplayRemote, RetIPodStateInfo),
    entry(DisplayRemote, SetIPodStateInfo),
    entry(DisplayRemote, GetPlayStatus),
    entry(DisplayRemote, RetPlayStatus),
    entry(DisplayRemote, SetCurrentPlayingTrack),
    entry(DisplayRemote, GetIndexedPlayingTrackInfo),
    entry(DisplayRemote, RetIndexedPlayingTrackInfo),
    entry(DisplayRemote, GetNumPlayingTracks),
    entry(DisplayRemote, RetNumPlayingTracks),
    entry(DisplayRemote, GetArtworkFormats),
    entry(DisplayRemote, RetArtworkFormats),
    entry(DisplayRemote, GetTrackArtworkData),
    entry(DisplayRemote, RetTrackArtworkData),
    entry(DisplayRemote, GetPowerBatteryState),
    entry(DisplayRemote, RetPowerBatteryState),
    entry(DisplayRemote, GetSoundCheckState),
    entry(DisplayRemote, RetSoundCheckState),
    entry(DisplayRemote, SetSoundCheckState),
    entry(DisplayRemote, GetTrackArtworkTimes),
    entry(DisplayRemote, RetTrackArtworkTimes),
    entry(DisplayRemote, CreateGeniusPlaylist),
    entry(DisplayRemote, IsGeniusAvailableForTrack),
};
static const char* strs_ext[] = {
    entry(ExtendedInterface, IPodAck),
    entry(ExtendedInterface, GetCurrentPlayingTrackChapterInfo),
    entry(ExtendedInterface, ReturnCurrentPlayingTrackChapterInfo),
    entry(ExtendedInterface, SetCurrentPlayingTrackChapter),
    entry(ExtendedInterface, GetCurrentPlayingTrackChapterPlayStatus),
    entry(ExtendedInterface, ReturnCurrentPlayingTrackChapterPlayStatus),
    entry(ExtendedInterface, GetCurrentPlayingTrackChapterName),
    entry(ExtendedInterface, ReturnCurrentPlayingTrackChapterName),
    entry(ExtendedInterface, GetAudiobookSpeed),
    entry(ExtendedInterface, RetAudiobookSpeed),
    entry(ExtendedInterface, SetAudiobookSpeed),
    entry(ExtendedInterface, GetIndexedPlayingTrackInfo),
    entry(ExtendedInterface, ReturnIndexedPlayingTrackInfo),
    entry(ExtendedInterface, GetArtworkFormats),
    entry(ExtendedInterface, RetArtworkFormats),
    entry(ExtendedInterface, GetTrackArtworkData),
    entry(ExtendedInterface, RetTrackArtworkData),
    entry(ExtendedInterface, RequestProtocolVersion),
    entry(ExtendedInterface, ReturnProtocolVersion),
    entry(ExtendedInterface, RequestIPodName),
    entry(ExtendedInterface, ReturnIPodName),
    entry(ExtendedInterface, ResetDBSelection),
    entry(ExtendedInterface, SelectDBRecord),
    entry(ExtendedInterface, GetNumberCategorizedDBRecords),
    entry(ExtendedInterface, ReturnNumberCategorizedDBRecords),
    entry(ExtendedInterface, RetrieveCategorizedDatabaseRecords),
    entry(ExtendedInterface, ReturnCategorizedDatabaseRecords),
    entry(ExtendedInterface, GetPlayStatus),
    entry(ExtendedInterface, ReturnPlayStatus),
    entry(ExtendedInterface, GetCurrentPlayingTrackIndex),
    entry(ExtendedInterface, ReturnCurrentPlayingTrackIndex),
    entry(ExtendedInterface, GetIndexedPlayingTrackTitle),
    entry(ExtendedInterface, ReturnIndexedPlayingTrackTitle),
    entry(ExtendedInterface, GetIndexedPlayingTrackArtistName),
    entry(ExtendedInterface, ReturnIndexedPlayingTrackArtistName),
    entry(ExtendedInterface, GetIndexedPlayingTrackAlbumName),
    entry(ExtendedInterface, ReturnIndexedPlayingTrackAlbumName),
    entry(ExtendedInterface, SetPlayStatusChangeNotification),
    entry(ExtendedInterface, PlayStatusChangeNotification),
    entry(ExtendedInterface, PlayCurrentSelection),
    entry(ExtendedInterface, PlayControl),
    entry(ExtendedInterface, GetTrackArtworkTimes),
    entry(ExtendedInterface, RetTrackArtworkTimes),
    entry(ExtendedInterface, GetShuffle),
    entry(ExtendedInterface, ReturnShuffle),
    entry(ExtendedInterface, SetShuffle),
    entry(ExtendedInterface, GetRepeat),
    entry(ExtendedInterface, ReturnRepeat),
    entry(ExtendedInterface, SetRepeat),
    entry(ExtendedInterface, SetDisplayImage),
    entry(ExtendedInterface, GetMonoDisplayImageLimits),
    entry(ExtendedInterface, ReturnMonoDisplayImageLimits),
    entry(ExtendedInterface, GetNumPlayingTracks),
    entry(ExtendedInterface, ReturnNumPlayingTracks),
    entry(ExtendedInterface, SetCurrentPlayingTrack),
    entry(ExtendedInterface, SelectSortDBRecord),
    entry(ExtendedInterface, GetColorDisplayImageLimits),
    entry(ExtendedInterface, ReturnColorDisplayImageLimits),
    entry(ExtendedInterface, ResetDBSelectionHierarchy),
    entry(ExtendedInterface, GetDBITunesInfo),
    entry(ExtendedInterface, RetDBITunesInfo),
    entry(ExtendedInterface, GetUIDTrackInfo),
    entry(ExtendedInterface, RetUIDTrackInfo),
    entry(ExtendedInterface, GetDBTrackInfo),
    entry(ExtendedInterface, RetDBTrackInfo),
    entry(ExtendedInterface, GetPBTrackInfo),
    entry(ExtendedInterface, RetPBTrackInfo),
    entry(ExtendedInterface, CreateGeniusPlaylist),
    entry(ExtendedInterface, RefreshGeniusPlaylist),
    entry(ExtendedInterface, IsGeniusAvailableForTrack),
    entry(ExtendedInterface, GetPlaylistInfo),
    entry(ExtendedInterface, RetPlaylistInfo),
    entry(ExtendedInterface, PrepareUIDList),
    entry(ExtendedInterface, PlayPreparedUIDList),
    entry(ExtendedInterface, GetArtworkTimes),
    entry(ExtendedInterface, RetArtworkTimes),
    entry(ExtendedInterface, GetArtworkData),
    entry(ExtendedInterface, RetArtworkData),
};
static const char* strs_da[] = {
    entry(DigitalAudio, AccessoryAck),
    entry(DigitalAudio, IPodAck),
    entry(DigitalAudio, GetAccessorySampleRateCaps),
    entry(DigitalAudio, RetAccessorySampleRateCaps),
    entry(DigitalAudio, TrackNewAudioAttributes),
    entry(DigitalAudio, SetVideoDelay),
};

#undef entry

#define entry(lingo, strs, size) [IAPLingoID_##lingo] = {#lingo, \
                                                         strs,   \
                                                         size}
static struct {
    const char*  lingo;
    const char** strs;
    size_t       size;
} strs[] = {
    entry(General, strs_general, array_size(strs_general)),
    entry(Microphone, NULL, 0),
    entry(SimpleRemote, strs_simple, array_size(strs_simple)),
    entry(DisplayRemote, strs_display, array_size(strs_display)),
    entry(ExtendedInterface, strs_ext, array_size(strs_ext)),
    entry(AccessoryPower, NULL, 0),
    entry(USBHostMode, NULL, 0),
    entry(RFTuner, NULL, 0),
    entry(AccessoryEqualizer, NULL, 0),
    entry(Sports, NULL, 0),
    entry(DigitalAudio, strs_da, array_size(strs_da)),
    entry(Storage, NULL, 0),
    entry(IPodOut, NULL, 0),
    entry(Location, NULL, 0),
};

#undef entry

const char* _iap_lingo_str_or_null(uint8_t lingo) {
    check_ret(lingo < array_size(strs), NULL);
    return strs[lingo].lingo;
}

const char* _iap_lingo_str(uint8_t lingo) {
    const char* ret = _iap_lingo_str_or_null(lingo);
    return ret ? ret : "?";
}

const char* _iap_command_str_or_null(uint8_t lingo, uint16_t command) {
    check_ret(lingo < array_size(strs), NULL);
    check_ret(command < strs[lingo].size, NULL);
    return strs[lingo].strs[command];
}

const char* _iap_command_str(uint8_t lingo, uint16_t command) {
    const char* ret = _iap_command_str_or_null(lingo, command);
    return ret ? ret : "?";
}

IAPBool _iap_span_is_str(const struct IAPSpan* span) {
    return span->size > 0 && span->ptr[span->size - 1] == '\0';
}

const char* _iap_span_as_str(const struct IAPSpan* span) {
    return _iap_span_is_str(span) ? (char*)span->ptr : "(invalid)";
}

#if !defined(IAP_LOGF_MUTED)
void _iap_dump_packet(uint8_t lingo, uint16_t command, int32_t trans_id, struct IAPSpan span) {
    const char* lingo_str   = _iap_lingo_str_or_null(lingo);
    const char* command_str = _iap_command_str_or_null(lingo, command);
    if(lingo_str == NULL) {
        IAP_LOGF("?(0x%02" PRIX8 ") trans=%" PRIi32, lingo, trans_id);
        return;
    } else if(command_str == NULL) {
        IAP_LOGF("%s:?(0x%02" PRIX8 ") trans=%" PRIi32, lingo_str, command, trans_id);
        return;
    } else {
        IAP_LOGF("%s:%s trans=%" PRIi32, lingo_str, command_str, trans_id);
    }

#define span_read(Type)                                                  \
    const struct Type* payload = iap_span_read(&span, sizeof(*payload)); \
    check_act(payload != NULL, return);

    switch(lingo) {
    case IAPLingoID_General:
        switch(command) {
        case IAPGeneralCommandID_IPodAck: {
            span_read(IAPIPodAckPayload);
            IAP_LOGF("  id=%s", _iap_command_str(lingo, payload->id));
            IAP_LOGF("  status=0x%02" PRIX8, payload->status);
        } break;
        case IAPGeneralCommandID_ReturnExtendedInterfaceMode: {
            span_read(IAPReturnExtendedInterfaceModePayload);
            IAP_LOGF("  mode=%" PRIu8, payload->is_ext_mode);
        } break;
        case IAPGeneralCommandID_ReturnIPodSoftwareVersion: {
            span_read(IAPReturnIPodSoftwareVersionPayload);
            IAP_LOGF("  version=%" PRIu8 ".%" PRIu8 ".%" PRIu8, payload->major, payload->minor, payload->revision);
        } break;
        case IAPGeneralCommandID_ReturnIPodSerialNum: {
            IAP_LOGF("  serial=%s", _iap_span_as_str(&span));
        } break;
        case IAPGeneralCommandID_ReturnIPodModelNum: {
            IAP_LOGF("  model=%s", _iap_span_as_str(&span));
        } break;
        case IAPGeneralCommandID_RequestLingoProtocolVersion: {
            span_read(IAPRequestLingoProtocolVersionPayload);
            IAP_LOGF("  lingo=%s", _iap_lingo_str(payload->lingo));
        } break;
        case IAPGeneralCommandID_ReturnLingoProtocolVersion: {
            span_read(IAPReturnLingoProtocolVersionPayload);
            IAP_LOGF("  lingo=%s", _iap_lingo_str(payload->lingo));
            IAP_LOGF("  version=%" PRIu8 ".%" PRIu8, payload->major, payload->minor);
        } break;
        case IAPGeneralCommandID_ReturnTransportMaxPayloadSize: {
            span_read(IAPReturnTransportMaxPayloadSizePayload);
            IAP_LOGF("  size=%" PRIu16, swap_16(payload->max_payload_size));
        } break;
        case IAPGeneralCommandID_IdentifyDeviceLingoes: {
            span_read(IAPIdentifyDeviceLingoesPayload);
            uint32_t bits = swap_32(payload->lingoes_bits);
            for(int i = 0; i < 16; i += 1) {
                if(bits & (1u << i)) {
                    IAP_LOGF("  supports %s", _iap_lingo_str(i));
                }
            }
            IAP_LOGF("  option=0x%02" PRIX32, swap_32(payload->options));
            IAP_LOGF("  device_id=0x%04" PRIX32, swap_32(payload->device_id));
        } break;
        case IAPGeneralCommandID_RetIPodOptions: {
            span_read(IAPRetIPodOptionsPayload);
            IAP_LOGF("  state=0x%08" PRIX64, swap_64(payload->state));
        } break;
        case IAPGeneralCommandID_GetIPodPreferences: {
            span_read(IAPGetIPodPreferencesPayload);
            IAP_LOGF("  class=0x%02" PRIX8, payload->class_id);
        } break;
        case IAPGeneralCommandID_RetIPodPreferences: {
            span_read(IAPSetIPodPreferencesPayload);
            IAP_LOGF("  class=0x%02X", payload->class_id);
            IAP_LOGF("  setting=0x%02X", payload->setting_id);
        } break;
        case IAPGeneralCommandID_SetIPodPreferences: {
            span_read(IAPSetIPodPreferencesPayload);
            IAP_LOGF("  class=0x%02" PRIX8, payload->class_id);
            IAP_LOGF("  setting=0x%02" PRIX8, payload->setting_id);
            IAP_LOGF("  roe=%" PRIu8, payload->restore_on_exit);
        } break;
        case IAPGeneralCommandID_SetUIMode: {
            span_read(IAPSetUIModePayload);
            IAP_LOGF("  mode=0x%02" PRIX8, payload->ui_mode);
        } break;
        case IAPGeneralCommandID_SetFIDTokenValues: {
            span_read(IAPSetFIDTokenValuesPayload);
            for(int i = 0; i < payload->num_token_values; i += 1) {
                check_act(span.size > sizeof(struct IAPFIDTokenValuesToken), return);
                struct IAPFIDTokenValuesToken* token_header = (void*)span.ptr;
                struct IAPSpan                 token_span   = {
                    span.ptr,
                    token_header->length + 1 /* length does not include sizeof itself */,
                };
                check_act(span.size >= token_span.size, return);
                iap_span_read(&span, token_span.size);

                switch(token_header->type << 8 | token_header->subtype) {
                case IAPFIDTokenTypes_Identify: {
                    /* IAPFIDTokenValuesIdentifyToken contains vla, need to parse manually */
                    const struct IAPFIDTokenValuesIdentifyTokenHead* token_head = iap_span_read(&token_span, sizeof(*token_head));
                    check_act(token_head != NULL, return);
                    print("accessory supported lingoes(%" PRIu8 "):", token_head->num_lingoes);
                    for(int i = 0; i < token_head->num_lingoes; i += 1) {
                        uint8_t lingo_id;
                        check_act(iap_span_read_8(&token_span, &lingo_id), return);
                        IAP_LOGF("  %s(0x%" PRIX8 ")", _iap_lingo_str(lingo_id), lingo_id);
                    }
                    const struct IAPFIDTokenValuesIdentifyTokenTail* token_tail = iap_span_read(&token_span, sizeof(*token_tail));
                    check_act(token_tail != NULL, return);
                    const uint32_t opt = swap_32(token_tail->device_option);
                    const uint32_t id  = swap_32(token_tail->device_id);
                    print("options=%04" PRIX32 " device_id=%04" PRIX32, opt, id);
                } break;
                case IAPFIDTokenTypes_AccCaps: {
                    const struct IAPFIDTokenValuesAccCapsToken* token = iap_span_read(&token_span, sizeof(*token));
                    check_act(token != NULL, return);
                    const uint64_t caps = swap_64(token->caps_bits);
                    print("accessory caps: %" PRIX64, caps);
                } break;
                case IAPFIDTokenTypes_AccInfo: {
                    const struct IAPFIDTokenValuesAccInfoToken* token = iap_span_read(&token_span, sizeof(*token));
                    check_act(token != NULL, return);
                    switch(token->info_type) {
                    case IAPFIDTokenValuesAccInfoTypes_AccName:
                        print("accessory name: %s", _iap_span_as_str(&token_span));
                        break;
                    case IAPFIDTokenValuesAccInfoTypes_FirmwareVersion:
                        check_act(token_span.size == 3, return);
                        print("accessory firmware version: %" PRIX8 ".%" PRIX8 ".%" PRIX8, token_span.ptr[0], token_span.ptr[1], token_span.ptr[2]);
                        break;
                    case IAPFIDTokenValuesAccInfoTypes_HardwareVersion:
                        check_act(token_span.size == 3, return);
                        print("accessory hardware version: %" PRIX8 ".%" PRIX8 ".%" PRIX8, token_span.ptr[0], token_span.ptr[1], token_span.ptr[2]);
                        break;
                    case IAPFIDTokenValuesAccInfoTypes_Manufacture:
                        print("accessory manufacture: %s", _iap_span_as_str(&token_span));
                        break;
                    case IAPFIDTokenValuesAccInfoTypes_ModelNumber:
                        print("accessory model number: %s", _iap_span_as_str(&token_span));
                        break;
                    case IAPFIDTokenValuesAccInfoTypes_SerialNumber:
                        print("accessory serial number: %s", _iap_span_as_str(&token_span));
                        break;
                    case IAPFIDTokenValuesAccInfoTypes_MaxPayloadSize: {
                        uint16_t val;
                        check_act(iap_span_read_16(&token_span, &val), return);
                        print("accessory max payload size: %" PRIu16, val);
                    } break;
                    case IAPFIDTokenValuesAccInfoTypes_AccStatus: {
                        uint32_t val;
                        check_act(iap_span_read_32(&token_span, &val), return);
                        print("accessory status: %" PRIX32, val);
                    } break;
                    case IAPFIDTokenValuesAccInfoTypes_RFCerts: {
                        uint32_t val;
                        check_act(iap_span_read_32(&token_span, &val), return);
                        print("accessory rf cert: %" PRIX32, val);
                    } break;
                    }
                } break;
                case IAPFIDTokenTypes_IPodPreference: {
                    const struct IAPFIDTokenValuesIPodPreferenceToken* token = iap_span_read(&token_span, sizeof(*token));
                    check_act(token != NULL, return);
                    print("accessory setting %" PRIX8 "=%" PRIX8, token->class_id, token->setting_id);
                } break;
                case IAPFIDTokenTypes_EAProtocol: {
                    const struct IAPFIDTokenValuesEAProtocolToken* token = iap_span_read(&token_span, sizeof(*token));
                    check_act(token != NULL, return);
                    print("ea protocol %" PRIX8 "=%s", token->protocol_index, _iap_span_as_str(&token_span));
                } break;
                case IAPFIDTokenTypes_BundleSeedIDPref: {
                    const struct IAPFIDTokenValuesBundleSeedIDPrefToken* token = iap_span_read(&token_span, sizeof(*token));
                    check_act(token != NULL, return);
                    print("bundle seed id %.10s", token->bundle_seed_id_string);
                } break;
                case IAPFIDTokenTypes_ScreenInfo: {
                    const struct IAPFIDTokenValuesScreenInfoToken* token = iap_span_read(&token_span, sizeof(*token));
                    check_act(token != NULL, return);
                    print("screen info:");
                    IAP_LOGF("  screen size(inch): %" PRIu16 "x%" PRIu16, swap_16(token->total_screen_width_inches), swap_16(token->total_screen_height_inches));
                    IAP_LOGF("  screen size(pixel): %" PRIu16 "x%" PRIu16, swap_16(token->total_screen_width_pixels), swap_16(token->total_screen_height_pixels));
                    IAP_LOGF("  ipod out size(pixel): %" PRIu16 "x%" PRIu16, swap_16(token->ipod_out_screen_width_pixels), swap_16(token->ipod_out_screen_height_pixels));
                    IAP_LOGF("  feature: %" PRIX8, token->screen_feature_mask);
                    IAP_LOGF("  gamma: %" PRIu8, token->screen_gamma_value);
                } break;
                case IAPFIDTokenTypes_EAProtocolMetadata: {
                    const struct IAPFIDTokenValuesEAProtocolMetadataToken* token = iap_span_read(&token_span, sizeof(*token));
                    check_act(token != NULL, return);
                    print("ea protocol metadata %" PRIX8 "=%" PRIX8, token->protocol_index, token->metadata_type);
                } break;
                case IAPFIDTokenTypes_AccDigitalAudioSampleRates: {
                    const struct IAPFIDTokenValuesAccDigitalAudioSampleRatesToken* token = iap_span_read(&token_span, sizeof(*token));
                    check_act(token != NULL, return);
                    print("accessory supported audio sample rates:");
                    while(token_span.size > 0) {
                        uint32_t rate;
                        check_act(iap_span_read_32(&token_span, &rate), return);
                        IAP_LOGF("  %" PRIu32, rate);
                    }
                } break;
                case IAPFIDTokenTypes_AccDigitalAudioVideoDelay: {
                    const struct IAPFIDTokenValuesAccDigitalAudioVideoDelayToken* token = iap_span_read(&token_span, sizeof(*token));
                    check_act(token != NULL, return);
                    print("accessory video delay: %" PRIu32, token->delay);
                } break;
                case IAPFIDTokenTypes_MicrophoneCaps: {
                    const struct IAPFIDTokenValuesMicrophoneCapsToken* token = iap_span_read(&token_span, sizeof(*token));
                    check_act(token != NULL, return);
                    print("accessory microphone caps: %" PRIX32, token->caps_bits);
                } break;
                default:
                    print("unknown fid %04" PRIX16, token_header->type << 8 | token_header->subtype);
                }
            }
        } break;
        case IAPGeneralCommandID_EndIDPS: {
            span_read(IAPEndIDPSPayload);
            IAP_LOGF("  status=0x%02" PRIX8, payload->status);
        } break;
        case IAPGeneralCommandID_GetIPodOptionsForLingo: {
            span_read(IAPGetIPodOptionsForLingoPayload);
            IAP_LOGF("  lingo=%s", _iap_lingo_str(payload->lingo_id));
        } break;
        case IAPGeneralCommandID_RetSupportedEventNotification: {
            span_read(IAPRetSupportedEventNotificationPayload);
            IAP_LOGF("  mask=0x%08" PRIX64, swap_64(payload->mask));
        } break;
        case IAPGeneralCommandID_SetAvailableCurrent: {
            span_read(IAPSetAvailableCurrentPayload);
            IAP_LOGF("  current=%" PRIu16 "mA", swap_16(payload->current_limit_ma));
        } break;
        case IAPGeneralCommandID_SetEventNotification: {
            span_read(IAPSetEventNotificationPayload);
            IAP_LOGF("  mask=0x%08" PRIX64, swap_64(payload->mask));
        } break;
        }
        break;
    case IAPLingoID_SimpleRemote:
        switch(command) {
        case IAPSimpleRemoteCommandID_ContextButtonStatus: {
            uint8_t bits;
            check_ret(iap_span_read_8(&span, &bits), );
            IAP_LOGF("  bits0=0x%02" PRIX8, bits);
            uint8_t index = 1;
            while(span.size > 0) {
                iap_span_read_8(&span, &bits);
                IAP_LOGF("  bits%d=0x%02" PRIX8, index, bits);
                index += 1;
            }
        } break;
        case IAPSimpleRemoteCommandID_IPodAck: {
            span_read(IAPIPodAckPayload);
            IAP_LOGF("  id=%s", _iap_command_str(lingo, payload->id));
            IAP_LOGF("  status=0x%02" PRIX8, payload->status);
        } break;
        }
        break;
    case IAPLingoID_DisplayRemote:
        switch(command) {
        case IAPDisplayRemoteCommandID_IPodAck: {
            span_read(IAPIPodAckPayload);
            IAP_LOGF("  id=%s", _iap_command_str(lingo, payload->id));
            IAP_LOGF("  status=0x%02" PRIX8, payload->status);
        } break;
        case IAPDisplayRemoteCommandID_SetCurrentEQProfileIndex: {
            span_read(IAPSetCurrentEQProfileIndexPayload);
            IAP_LOGF("  index=%" PRIu32, swap_32(payload->index));
            IAP_LOGF("  roe=%" PRIu8, payload->restore_on_exit);
        } break;
        case IAPDisplayRemoteCommandID_SetRemoteEventNotification: {
            span_read(IAPSetRemoteEventNotificationPayload);
            IAP_LOGF("  mask=0x%04" PRIX32, swap_32(payload->mask));
        } break;
        case IAPDisplayRemoteCommandID_RemoteEventNotification: {
            check_ret(span.size >= 1, );
            IAP_LOGF("  type=0x%02" PRIX8, span.ptr[0]);
            switch(span.ptr[0]) {
            case IAPIPodStateType_TrackTimePositionMSec: {
                span_read(IAPIPodStateTrackTimePositionMSecPayload);
                IAP_LOGF("  position=%" PRIu32 "ms", swap_32(payload->position_ms));
            } break;
            case IAPIPodStateType_TrackPlaybackIndex: {
                span_read(IAPIPodStateTrackPlaybackIndexPayload);
                IAP_LOGF("  index=%" PRIu32, swap_32(payload->index));
            } break;
            case IAPIPodStateType_ChapterIndex: {
                span_read(IAPIPodStateChapterIndexPayload);
                IAP_LOGF("  chapter_index=%" PRIu16, swap_16(payload->chapter_index));
                IAP_LOGF("  chapter_count=%" PRIu16, swap_16(payload->chapter_count));
            } break;
            case IAPIPodStateType_PlayStatus: {
                span_read(IAPIPodStatePlayStatusPayload);
                IAP_LOGF("  play_status=%" PRIu8, payload->status);
            } break;
            case IAPIPodStateType_Volume: {
                span_read(IAPIPodStateVolumePayload);
                IAP_LOGF("  mute=%" PRIu8, payload->mute_state);
                IAP_LOGF("  volume=%" PRIu8, payload->ui_volume);
            } break;
            case IAPIPodStateType_Power: {
                span_read(IAPIPodStatePowerPayload);
                IAP_LOGF("  power_state=%" PRIu8, payload->power_state);
                IAP_LOGF("  battery_level=%" PRIu8, payload->battery_level);
            } break;
            case IAPIPodStateType_EQSetting: {
                span_read(IAPIPodStateEQSettingPayload);
                IAP_LOGF("  eq_index=%" PRIu32, swap_32(payload->eq_index));
            } break;
            case IAPIPodStateType_ShuffleSetting: {
                span_read(IAPIPodStateShuffleSettingPayload);
                IAP_LOGF("  shuffle_state=%" PRIu8, payload->shuffle_state);
            } break;
            case IAPIPodStateType_RepeatSetting: {
                span_read(IAPIPodStateRepeatSettingPayload);
                IAP_LOGF("  repeat_state=%" PRIu8, payload->repeat_state);
            } break;
            case IAPIPodStateType_DateTimeSetting: {
                span_read(IAPIPodStateDateTimeSettingPayload);
                IAP_LOGF("  time=%04" PRIu16 "-%02" PRIu8 "-%02" PRIu8 " %02" PRIu8 ":%02" PRIu8, swap_16(payload->year), payload->month, payload->day, payload->hour, payload->minute);
            } break;
            case IAPIPodStateType_AlarmSetting: {
                span_read(IAPIPodStateAlarmSettingPayload);
                IAP_LOGF("  alarm");
            } break;
            case IAPIPodStateType_BacklightLevel: {
                span_read(IAPIPodStateBacklightLevelPayload);
                IAP_LOGF("  backlight_level=%" PRIu8, payload->level);
            } break;
            case IAPIPodStateType_HoldSwitchState: {
                span_read(IAPIPodStateHoldSwitchStatePayload);
                IAP_LOGF("  hold_switch_state=%" PRIu8, payload->state);
            } break;
            case IAPIPodStateType_SoundCheckState: {
                span_read(IAPIPodStateSoundCheckStatePayload);
                IAP_LOGF("  sound_check_state=%" PRIu8, payload->state);
            } break;
            case IAPIPodStateType_AudiobookSpeeed: {
                span_read(IAPIPodStateAudiobookSpeeedPayload);
                IAP_LOGF("  audio_book_speed=%" PRIu8, payload->speed);
            } break;
            case IAPIPodStateType_TrackTimePositionSec: {
                span_read(IAPIPodStateTrackTimePositionSecPayload);
                IAP_LOGF("  position=%us", swap_16(payload->position_s));
            } break;
            case IAPIPodStateType_AbsoluteVolume: {
                span_read(IAPIPodStateAbsoluteVolumePayload);
                IAP_LOGF("  mute=%" PRIu8, payload->mute_state);
                IAP_LOGF("  volume=%" PRIu8, payload->ui_volume);
                IAP_LOGF("  absolute_volume=%" PRIu8, payload->absolute_volume);
            } break;
            case IAPIPodStateType_TrackCaps: {
                span_read(IAPIPodStateTrackCapsPayload);
                IAP_LOGF("  track_caps=0x%04" PRIX32, swap_32(payload->caps));
            } break;
            case IAPIPodStateType_PlaybackEngineContents: {
                span_read(IAPIPodStatePlaybackEngineContentsPayload);
                IAP_LOGF("  playback_engine_contents=%" PRIu32, swap_32(payload->count));
            } break;
            }
        } break;
        case IAPDisplayRemoteCommandID_GetIPodStateInfo: {
            span_read(IAPGetIPodStateInfoPayload);
            IAP_LOGF("  type=0x%02" PRIX8, payload->type);
        } break;
        case IAPDisplayRemoteCommandID_RetIPodStateInfo: {
            check_ret(span.size > 1, );
            IAP_LOGF("  type=0x%02" PRIX8, span.ptr[0]);
            switch(span.ptr[0]) {
            case IAPIPodStateType_TrackTimePositionMSec: {
                span_read(IAPIPodStateTrackTimePositionMSecPayload);
                IAP_LOGF("  position=%" PRIu32 "ms", swap_32(payload->position_ms));
            } break;
            case IAPIPodStateType_TrackPlaybackIndex: {
                span_read(IAPIPodStateTrackPlaybackIndexPayload);
                IAP_LOGF("  index=%" PRIu32, swap_32(payload->index));
            } break;
            case IAPIPodStateType_ChapterIndex: {
                span_read(IAPIPodStateChapterIndexPayload);
                IAP_LOGF("  index=%" PRIu32, swap_32(payload->index));
                IAP_LOGF("  cindex=%" PRIu16, swap_16(payload->chapter_index));
                IAP_LOGF("  ccount=%" PRIu16, swap_16(payload->chapter_count));
            } break;
            case IAPIPodStateType_PlayStatus: {
                span_read(IAPIPodStatePlayStatusPayload);
                IAP_LOGF("  status=%" PRIu8, payload->status);
            } break;
            case IAPIPodStateType_Volume: {
                span_read(IAPIPodStateVolumePayload);
                IAP_LOGF("  ui_volume=%" PRIu8, payload->ui_volume);
                IAP_LOGF("  mute_state=%" PRIu8, payload->mute_state);
            } break;
            case IAPIPodStateType_Power: {
                span_read(IAPIPodStatePowerPayload);
                IAP_LOGF("  power_state=0x%02" PRIX8, payload->power_state);
                IAP_LOGF("  battery_level=%" PRIu8, payload->battery_level);
            } break;
            case IAPIPodStateType_EQSetting: {
                span_read(IAPIPodStateEQSettingPayload);
                IAP_LOGF("  eq_index=%" PRIu32, swap_32(payload->eq_index));
            } break;
            case IAPIPodStateType_ShuffleSetting: {
                span_read(IAPIPodStateShuffleSettingPayload);
                IAP_LOGF("  shuffle_state=%" PRIu8, payload->shuffle_state);
            } break;
            case IAPIPodStateType_RepeatSetting: {
                span_read(IAPIPodStateRepeatSettingPayload);
                IAP_LOGF("  repeat_state=%" PRIu8, payload->repeat_state);
            } break;
            case IAPIPodStateType_DateTimeSetting: {
                span_read(IAPIPodStateDateTimeSettingPayload);
                IAP_LOGF("  time=%04" PRIu16 "-%02" PRIu8 "-%02" PRIu8 " %02" PRIu8 ":%02" PRIu8, swap_16(payload->year), payload->month, payload->day, payload->hour, payload->minute);
            } break;
            case IAPIPodStateType_AlarmSetting: {
            } break;
            case IAPIPodStateType_BacklightLevel: {
                span_read(IAPIPodStateBacklightLevelPayload);
                IAP_LOGF("  level=%" PRIu8, payload->level);
            } break;
            case IAPIPodStateType_HoldSwitchState: {
                span_read(IAPIPodStateHoldSwitchStatePayload);
                IAP_LOGF("  state=%" PRIu8, payload->state);
            } break;
            case IAPIPodStateType_SoundCheckState: {
                span_read(IAPIPodStateSoundCheckStatePayload);
                IAP_LOGF("  state=%" PRIu8, payload->state);
            } break;
            case IAPIPodStateType_AudiobookSpeeed: {
                span_read(IAPIPodStateAudiobookSpeeedPayload);
                IAP_LOGF("  speed=%" PRIu8, payload->speed);
            } break;
            case IAPIPodStateType_TrackTimePositionSec: {
                span_read(IAPIPodStateTrackTimePositionSecPayload);
                IAP_LOGF("  position=%" PRIu16 "s", swap_16(payload->position_s));
            } break;
            case IAPIPodStateType_AbsoluteVolume: {
                span_read(IAPIPodStateAbsoluteVolumePayload);
                IAP_LOGF("  mute=%" PRIu8, payload->mute_state);
                IAP_LOGF("  ui_volume=%" PRIu8, payload->ui_volume);
                IAP_LOGF("  absolute_volume=%" PRIu8, payload->absolute_volume);
            } break;
            case IAPIPodStateType_TrackCaps: {
                span_read(IAPIPodStateTrackCapsPayload);
                IAP_LOGF("  caps=0x%04" PRIX32, swap_32(payload->caps));
            } break;
            case IAPIPodStateType_PlaybackEngineContents: {
                span_read(IAPIPodStatePlaybackEngineContentsPayload);
                IAP_LOGF("  count=%" PRIu32, swap_32(payload->count));
            } break;
            }
        } break;
        case IAPDisplayRemoteCommandID_RetPlayStatus: {
            span_read(IAPRetPlayStatusPayload);
            IAP_LOGF("  state=0x%02" PRIX8, payload->state);
            IAP_LOGF("  index=%" PRIu32, swap_32(payload->track_index));
            IAP_LOGF("  pos=%" PRIu32 "ms", swap_32(payload->track_pos_ms));
            IAP_LOGF("  total=%" PRIu32 "ms", swap_32(payload->track_total_ms));
        } break;
        case IAPDisplayRemoteCommandID_GetIndexedPlayingTrackInfo: {
            span_read(IAPGetIndexedPlayingTrackInfoPayload);
            IAP_LOGF("  type=0x%02" PRIX8, payload->type);
            IAP_LOGF("  track=%" PRIu32, swap_32(payload->track_index));
            IAP_LOGF("  chapter=%" PRIu16, swap_16(payload->chapter_index));
        } break;
        case IAPDisplayRemoteCommandID_RetIndexedPlayingTrackInfo: {
            check_ret(span.size > 1, );
            IAP_LOGF("  type=0x%02" PRIX8, span.ptr[0]);
            switch(span.ptr[0]) {
            case IAPIndexedPlayingTrackInfoType_TrackCapsInfo: {
                span_read(IAPRetIndexedPlayingTrackInfoTrackCapsInfoPayload);
                IAP_LOGF("  caps=0x%04" PRIX32, swap_32(payload->track_caps));
                IAP_LOGF("  total=%" PRIu32 "ms", swap_32(payload->track_total_ms));
                IAP_LOGF("  chapter_count=%" PRIu16, swap_16(payload->chapter_count));
            } break;
            case IAPIndexedPlayingTrackInfoType_ChapterTimeName: {
                span_read(IAPRetIndexedPlayingTrackInfoChapterTimeNamePayload);
                IAP_LOGF("  offset=%" PRIu32 "ms", swap_32(payload->offset_ms));
                IAP_LOGF("  name=%s", _iap_span_as_str(&span));
            } break;
            case IAPIndexedPlayingTrackInfoType_ArtistName:
            case IAPIndexedPlayingTrackInfoType_AlbumName:
            case IAPIndexedPlayingTrackInfoType_GenreName:
            case IAPIndexedPlayingTrackInfoType_ComposerName: {
                IAP_LOGF("  str=%s", _iap_span_as_str(&span));
            } break;
            case IAPIndexedPlayingTrackInfoType_Lyrics: {
                span_read(IAPRetIndexedPlayingTrackInfoLyricsPayload);
                IAP_LOGF("  info=0x%02" PRIX8, payload->info_bits);
                IAP_LOGF("  index=%" PRIu16, swap_16(payload->index));
                IAP_LOGF("  lyrics=%s", _iap_span_as_str(&span));
            } break;
            case IAPIndexedPlayingTrackInfoType_ArtworkCount: {
                span_read(IAPRetIndexedPlayingTrackInfoArtworkCountPayload);
                while(span.size >= sizeof(struct IAPArtworkCount)) {
                    const struct IAPArtworkCount* count = iap_span_read(&span, sizeof(*count));
                    IAP_LOGF("  format=%" PRIu16 ", count=%" PRIu16, swap_16(count->format), swap_16(count->count));
                }
            } break;
            }
        } break;
        case IAPDisplayRemoteCommandID_RetNumPlayingTracks: {
            span_read(IAPRetNumPlayingTracksPayload);
            IAP_LOGF("  tracks=%" PRIu32, swap_32(payload->num_playing_tracks));
        } break;
        case IAPDisplayRemoteCommandID_RetArtworkFormats: {
            while(span.size >= sizeof(struct IAPArtworkFormat)) {
                const struct IAPArtworkFormat* format = iap_span_read(&span, sizeof(*format));
                IAP_LOGF("  id=%" PRIu16 ", format=%" PRIu8 ", width=%" PRIu16 ", height=%" PRIu16, swap_16(format->format_id), format->pixel_format, swap_16(format->image_width), swap_16(format->image_height));
            }
        } break;
        case IAPDisplayRemoteCommandID_GetTrackArtworkData: {
            span_read(IAPGetTrackArtworkDataPayload);
            IAP_LOGF("  index=%" PRIu32, swap_32(payload->track_index));
            IAP_LOGF("  format=%" PRIu8, payload->format_id);
            IAP_LOGF("  offset=%" PRIu32 "ms", swap_32(payload->offset_ms));
        } break;
        case IAPDisplayRemoteCommandID_RetTrackArtworkData: {
            uint16_t index;
            check_ret(iap_span_peek_16(&span, &index), );
            if(index == 0) {
                span_read(IAPRetTrackArtworkDataFirstPayload);
                IAP_LOGF("  index=%" PRIu16, swap_16(payload->index));
                IAP_LOGF("  format=%" PRIu16, swap_16(payload->pixel_format));
                IAP_LOGF("  width=%" PRIu16, swap_16(payload->pixel_width));
                IAP_LOGF("  height=%" PRIu16, swap_16(payload->pixel_height));
            } else {
                span_read(IAPRetTrackArtworkDataSubsequenctPayload);
                IAP_LOGF("  index=%" PRIu16, swap_16(payload->index));
            }
        } break;
        case IAPDisplayRemoteCommandID_RetPowerBatteryState: {
            span_read(IAPRetPowerBatteryStatePayload);
            IAP_LOGF("  power_state=%" PRIu8, payload->power_state);
            IAP_LOGF("  battery_level=%" PRIu8, payload->battery_level);
        } break;
        case IAPDisplayRemoteCommandID_GetTrackArtworkTimes: {
            span_read(IAPGetTrackArtworkTimesPayload);
            IAP_LOGF("  index=%" PRIu32, swap_32(payload->track_index));
            IAP_LOGF("  format=%" PRIu16, swap_16(payload->format_id));
            IAP_LOGF("  artwork_index=%" PRIu32, swap_32(payload->artwork_index));
            IAP_LOGF("  artwork_count=%" PRIu32, swap_32(payload->artwork_count));
        } break;
        case IAPDisplayRemoteCommandID_RetTrackArtworkTimes: {
            while(span.size >= sizeof(uint16_t)) {
                uint16_t time;
                iap_span_read_16(&span, &time);
                IAP_LOGF("  time=%" PRIu16, time);
            }
        } break;
        }
        break;
    case IAPLingoID_ExtendedInterface:
        switch(command) {
        case IAPExtendedInterfaceCommandID_IPodAck: {
            span_read(IAPExtendedIPodAckPayload);
            IAP_LOGF("  id=%s", _iap_command_str(lingo, swap_16(payload->id)));
            IAP_LOGF("  status=0x%02" PRIX8, payload->status);
        } break;
        case IAPExtendedInterfaceCommandID_ReturnCurrentPlayingTrackChapterInfo: {
            span_read(IAPReturnCurrentPlayingTrackChapterInfoPayload);
            IAP_LOGF("  count=%" PRIu32, swap_32(payload->count));
            IAP_LOGF("  index=%" PRIu32, swap_32(payload->index));
        } break;
        case IAPExtendedInterfaceCommandID_RetAudiobookSpeed: {
            span_read(IAPRetAudiobookSpeedPayload);
            IAP_LOGF("  speed=0x%02" PRIX8, payload->speed);
        } break;
        case IAPExtendedInterfaceCommandID_GetIndexedPlayingTrackInfo: {
            span_read(IAPExtendedGetIndexedPlayingTrackInfoPayload);
            IAP_LOGF("  type=0x%02" PRIX8, payload->type);
            IAP_LOGF("  track=%" PRIu32, swap_32(payload->track_index));
            IAP_LOGF("  chapter=%" PRIu16, swap_16(payload->chapter_index));
        } break;
        case IAPExtendedInterfaceCommandID_ReturnIndexedPlayingTrackInfo: {
            check_ret(span.size > 1, );
            IAP_LOGF("  type=0x%02" PRIX8, span.ptr[0]);
            switch(span.ptr[0]) {
            case IAPExtendedIndexedPlayingTrackInfoType_TrackCapsInfo: {
                span_read(IAPExtendedRetIndexedPlayingTrackInfoTrackCapsInfoPayload);
                IAP_LOGF("  caps=0x%04" PRIX32, swap_32(payload->track_caps));
                IAP_LOGF("  total=%" PRIu32 "ms", swap_32(payload->track_total_ms));
                IAP_LOGF("  chapter_count=%" PRIu16, swap_16(payload->chapter_count));
            } break;
            case IAPExtendedIndexedPlayingTrackInfoType_PodcastName: {
                IAP_LOGF("  str=%s", _iap_span_as_str(&span));
            } break;
            case IAPExtendedIndexedPlayingTrackInfoType_TrackReleaseDate: {
                span_read(IAPExtendedRetIndexedPlayingTrackInfoTrackReleaseDatePayload);
                IAP_LOGF("  release=%04" PRIu16 "-%02" PRIu8 "-%02" PRIu8 " %02" PRIu8 ":%02" PRIu8 ".%02" PRIu8, swap_16(payload->year), payload->month, payload->day, payload->hours, payload->minutes, payload->seconds);
            } break;
            case IAPExtendedIndexedPlayingTrackInfoType_TrackDescription: {
                span_read(IAPExtendedRetIndexedPlayingTrackInfoTrackDescriptionPayload);
                IAP_LOGF("  info=0x%02" PRIX8, payload->info_bits);
                IAP_LOGF("  index=%" PRIu8, swap_16(payload->index));
                IAP_LOGF("  desc=%s", _iap_span_as_str(&span));
            } break;
            case IAPExtendedIndexedPlayingTrackInfoType_TrackSongLyrics: {
                span_read(IAPExtendedRetIndexedPlayingTrackInfoTrackSongLyricsPayload);
                IAP_LOGF("  info=0x%02" PRIX8, payload->info_bits);
                IAP_LOGF("  index=%u" PRIu16, swap_16(payload->index));
                IAP_LOGF("  lyrics=%s", _iap_span_as_str(&span));
            } break;
            case IAPExtendedIndexedPlayingTrackInfoType_TrackGenre: {
                IAP_LOGF("  str=%s", _iap_span_as_str(&span));
            } break;
            case IAPExtendedIndexedPlayingTrackInfoType_TrackComposer: {
                IAP_LOGF("  str=%s", _iap_span_as_str(&span));
            } break;
            case IAPExtendedIndexedPlayingTrackInfoType_TrackArtworkCount: {
                span_read(IAPRetIndexedPlayingTrackInfoArtworkCountPayload);
                while(span.size >= sizeof(struct IAPArtworkCount)) {
                    const struct IAPArtworkCount* count = iap_span_read(&span, sizeof(*count));
                    IAP_LOGF("  format=%" PRIu16 ", count=%" PRIu16, swap_16(count->format), swap_16(count->count));
                }
            } break;
            }
        } break;
        case IAPExtendedInterfaceCommandID_RetArtworkFormats: {
            while(span.size >= sizeof(struct IAPArtworkFormat)) {
                const struct IAPArtworkFormat* format = iap_span_read(&span, sizeof(*format));
                IAP_LOGF("  id=%" PRIu16 ", format=%" PRIu8 ", width=%" PRIu16 ", height=%" PRIu16, swap_16(format->format_id), format->pixel_format, swap_16(format->image_width), swap_16(format->image_height));
            }
        } break;
        case IAPExtendedInterfaceCommandID_GetTrackArtworkData: {
            span_read(IAPGetTrackArtworkDataPayload);
            IAP_LOGF("  index=%" PRIu32, swap_32(payload->track_index));
            IAP_LOGF("  format=%" PRIu8, payload->format_id);
            IAP_LOGF("  offset=%" PRIu32 "ms", swap_32(payload->offset_ms));
        } break;
        case IAPExtendedInterfaceCommandID_RetTrackArtworkData: {
            uint16_t index;
            check_ret(iap_span_peek_16(&span, &index), );
            if(index == 0) {
                span_read(IAPRetTrackArtworkDataFirstPayload);
                IAP_LOGF("  index=%" PRIu16, swap_16(payload->index));
                IAP_LOGF("  format=%" PRIu16, swap_16(payload->pixel_format));
                IAP_LOGF("  width=%" PRIu16, swap_16(payload->pixel_width));
                IAP_LOGF("  height=%" PRIu16, swap_16(payload->pixel_height));
            } else {
                span_read(IAPRetTrackArtworkDataSubsequenctPayload);
                IAP_LOGF("  index=%" PRIu16, swap_16(payload->index));
            }
        } break;
        case IAPExtendedInterfaceCommandID_GetNumberCategorizedDBRecords: {
            span_read(IAPGetNumberCategorizedDBRecordsPayload);
            IAP_LOGF("  type=0x%02" PRIX8, payload->type);
        } break;
        case IAPExtendedInterfaceCommandID_ReturnNumberCategorizedDBRecords: {
            span_read(IAPReturnNumberCategorizedDBRecordsPayload);
            IAP_LOGF("  count=%" PRIu32, swap_32(payload->count));
        } break;
        case IAPExtendedInterfaceCommandID_ReturnPlayStatus: {
            span_read(IAPExtendedRetPlayStatusPayload);
            IAP_LOGF("  state=0x%02" PRIX8, payload->state);
            IAP_LOGF("  pos=%" PRIu32 "ms", swap_32(payload->track_pos_ms));
            IAP_LOGF("  total=%" PRIu32 "ms", swap_32(payload->track_total_ms));
        } break;
        case IAPExtendedInterfaceCommandID_ReturnCurrentPlayingTrackIndex: {
            span_read(IAPReturnCurrentPlayingTrackIndexPayload);
            IAP_LOGF("  index=%" PRIu32, swap_32(payload->index));
        } break;
        case IAPExtendedInterfaceCommandID_GetIndexedPlayingTrackTitle:
        case IAPExtendedInterfaceCommandID_GetIndexedPlayingTrackArtistName:
        case IAPExtendedInterfaceCommandID_GetIndexedPlayingTrackAlbumName: {
            span_read(IAPGetIndexedPlayingTrackStringPayload);
            IAP_LOGF("  index=%" PRIu32, swap_32(payload->index));
        } break;
        case IAPExtendedInterfaceCommandID_ReturnIndexedPlayingTrackTitle:
        case IAPExtendedInterfaceCommandID_ReturnIndexedPlayingTrackArtistName:
        case IAPExtendedInterfaceCommandID_ReturnIndexedPlayingTrackAlbumName: {
            IAP_LOGF("  str=%s", _iap_span_as_str(&span));
        } break;
        case IAPExtendedInterfaceCommandID_SetPlayStatusChangeNotification: {
            if(span.size == sizeof(struct IAPSetPlayStatusChangeNotification1BytePayload)) {
                span_read(IAPSetPlayStatusChangeNotification1BytePayload);
                IAP_LOGF("  enable=%" PRIu8, payload->enable);
            } else {
                span_read(IAPSetPlayStatusChangeNotification4BytesPayload);
                IAP_LOGF("  mask=0x%08" PRIX32, swap_32(payload->mask));
            }
        } break;
        case IAPExtendedInterfaceCommandID_PlayControl: {
            span_read(IAPPlayControlPayload);
            IAP_LOGF("  code=0x%02" PRIX8, payload->code);
        } break;
        case IAPExtendedInterfaceCommandID_ReturnShuffle: {
            span_read(IAPReturnShufflePayload);
            IAP_LOGF("  mode=%" PRIu8, payload->mode);
        } break;
        case IAPExtendedInterfaceCommandID_SetShuffle: {
            span_read(IAPSetShufflePayload);
            IAP_LOGF("  mode=%" PRIu8, payload->mode);
            IAP_LOGF("  roe=%" PRIu8, payload->restore_on_exit);
        } break;
        case IAPExtendedInterfaceCommandID_ReturnRepeat: {
            span_read(IAPReturnRepeatPayload);
            IAP_LOGF("  mode=%" PRIu8, payload->mode);
        } break;
        case IAPExtendedInterfaceCommandID_SetRepeat: {
            span_read(IAPSetRepeatPayload);
            IAP_LOGF("  mode=%" PRIu8, payload->mode);
            IAP_LOGF("  roe=%" PRIu8, payload->restore_on_exit);
        } break;
        case IAPExtendedInterfaceCommandID_SetDisplayImage: {
            uint16_t index;
            check_ret(iap_span_peek_16(&span, &index), );
            IAP_LOGF("  index=%" PRIu16, index);
            if(index == 0) {
                span_read(IAPSetDisplayImageFirstPayload);
                IAP_LOGF("  format=%02" PRIX8, payload->pixel_format);
                IAP_LOGF("  width=%" PRIu16, swap_16(payload->pixel_width));
                IAP_LOGF("  height=%" PRIu16, swap_16(payload->pixel_height));
            }
        } break;
        case IAPExtendedInterfaceCommandID_ReturnNumPlayingTracks: {
            span_read(IAPRetNumPlayingTracksPayload);
            IAP_LOGF("  tracks=%" PRIu32, swap_32(payload->num_playing_tracks));
        } break;
        case IAPExtendedInterfaceCommandID_SetCurrentPlayingTrack: {
            span_read(IAPSetCurrentPlayingTrackPayload);
            IAP_LOGF("  index=%" PRIu32, swap_32(payload->index));
        } break;
        case IAPExtendedInterfaceCommandID_ReturnColorDisplayImageLimits: {
            span_read(IAPColorDisplayImageLimit);
            IAP_LOGF("  width=%" PRIu16, swap_16(payload->max_width));
            IAP_LOGF("  height=%" PRIu16, swap_16(payload->max_height));
            IAP_LOGF("  format=0x%02" PRIX8, payload->pixel_format);
        } break;
        }
        break;
    case IAPLingoID_DigitalAudio:
        switch(command) {
        case IAPDigitalAudioCommandID_AccessoryAck: {
            span_read(IAPAccAckPayload);
            IAP_LOGF("  id=%s", _iap_command_str(lingo, payload->id));
            IAP_LOGF("  status=0x%02" PRIX8, payload->status);
        } break;
        case IAPDigitalAudioCommandID_RetAccessorySampleRateCaps: {
            while(span.size >= sizeof(uint32_t)) {
                uint32_t sample_rate;
                iap_span_read_32(&span, &sample_rate);
                IAP_LOGF("  rate=%" PRIu32, sample_rate);
            }
        } break;
        }
        break;
    }
#undef span_read
}
#else
void _iap_dump_packet(uint8_t lingo, uint16_t command, int32_t trans_id, struct IAPSpan span) {
    (void)lingo;
    (void)command;
    (void)trans_id;
    (void)span;
}
#endif
