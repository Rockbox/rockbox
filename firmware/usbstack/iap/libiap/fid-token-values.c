#include "endian.h"
#include "iap.h"
#include "macros.h"
#include "span.h"
#include "spec/iap.h"

#define pack_accepted(Ack)                                          \
    struct Ack* const ack = iap_span_alloc(response, sizeof(*ack)); \
    check_ret(ack != NULL, -IAPAckStatus_EOutOfResource);           \
    ack->length  = sizeof(struct Ack) - 1;                          \
    ack->type    = token_header->type;                              \
    ack->subtype = token_header->subtype;                           \
    ack->status  = IAPFIDTokenValuesIdentifyAckStatus_Accepted;

int _iap_hanlde_set_fid_token_values(struct IAPSpan* request, struct IAPSpan* response) {
    const struct IAPSetFIDTokenValuesPayload* request_payload = iap_span_read(request, sizeof(*request_payload));
    check_ret(request_payload != NULL, -IAPAckStatus_EBadParameter);
    check_ret(iap_span_write_8(response, request_payload->num_token_values), -IAPAckStatus_EOutOfResource);
    for(int i = 0; i < request_payload->num_token_values; i += 1) {
        check_ret(request->size > sizeof(struct IAPFIDTokenValuesToken), -IAPAckStatus_EBadParameter);
        struct IAPFIDTokenValuesToken* token_header = (void*)request->ptr;
        struct IAPSpan                 token_span   = {
            request->ptr,
            token_header->length + 1 /* length does not include sizeof itself */,
        };
        check_ret(request->size >= token_span.size, -IAPAckStatus_EBadParameter);
        iap_span_read(request, token_span.size);

        switch(token_header->type << 8 | token_header->subtype) {
        case IAPFIDTokenTypes_Identify: {
            /* IAPFIDTokenValuesIdentifyToken contains vla, need to parse manually */
            const struct IAPFIDTokenValuesIdentifyTokenHead* token_head = iap_span_read(&token_span, sizeof(*token_head));
            check_ret(token_head != NULL, -IAPAckStatus_EBadParameter);
            for(int i = 0; i < token_head->num_lingoes; i += 1) {
                uint8_t lingo_id;
                check_ret(iap_span_read_8(&token_span, &lingo_id), -IAPAckStatus_EBadParameter);
            }
            const struct IAPFIDTokenValuesIdentifyTokenTail* token_tail = iap_span_read(&token_span, sizeof(*token_tail));
            check_ret(token_tail != NULL, -IAPAckStatus_EBadParameter);
            const uint32_t opt = swap_32(token_tail->device_option);
            const uint32_t id  = swap_32(token_tail->device_id);
            pack_accepted(IAPFIDTokenValuesIdentifyAck);
            if(opt != IAPIdentifyDeviceLingoesOptions_ImmediateAuth) {
                ack->status = IAPFIDTokenValuesIdentifyAckStatus_RequiredFailed;
            }
            (void)id;
        } break;
        case IAPFIDTokenTypes_AccCaps: {
            const struct IAPFIDTokenValuesAccCapsToken* token = iap_span_read(&token_span, sizeof(*token));
            check_ret(token != NULL, -IAPAckStatus_EBadParameter);
            const uint64_t caps = swap_64(token->caps_bits);
            (void)caps;
            pack_accepted(IAPFIDTokenValuesAccCapsAck);
        } break;
        case IAPFIDTokenTypes_AccInfo: {
            const struct IAPFIDTokenValuesAccInfoToken* token = iap_span_read(&token_span, sizeof(*token));
            check_ret(token != NULL, -IAPAckStatus_EBadParameter);
            switch(token->info_type) {
            case IAPFIDTokenValuesAccInfoTypes_AccName:
                break;
            case IAPFIDTokenValuesAccInfoTypes_FirmwareVersion:
                check_ret(token_span.size == 3, -IAPAckStatus_EBadParameter);
                break;
            case IAPFIDTokenValuesAccInfoTypes_HardwareVersion:
                check_ret(token_span.size == 3, -IAPAckStatus_EBadParameter);
                break;
            case IAPFIDTokenValuesAccInfoTypes_Manufacture:
                break;
            case IAPFIDTokenValuesAccInfoTypes_ModelNumber:
                break;
            case IAPFIDTokenValuesAccInfoTypes_SerialNumber:
                break;
            case IAPFIDTokenValuesAccInfoTypes_MaxPayloadSize: {
                uint16_t val;
                check_ret(iap_span_read_16(&token_span, &val), -IAPAckStatus_EBadParameter);
            } break;
            case IAPFIDTokenValuesAccInfoTypes_AccStatus: {
                uint32_t val;
                check_ret(iap_span_read_32(&token_span, &val), -IAPAckStatus_EBadParameter);
            } break;
            case IAPFIDTokenValuesAccInfoTypes_RFCerts: {
                uint32_t val;
                check_ret(iap_span_read_32(&token_span, &val), -IAPAckStatus_EBadParameter);
            } break;
            }
            pack_accepted(IAPFIDTokenValuesAccInfoAck);
            ack->info_type = token->info_type;
        } break;
        case IAPFIDTokenTypes_IPodPreference: {
            const struct IAPFIDTokenValuesIPodPreferenceToken* token = iap_span_read(&token_span, sizeof(*token));
            check_ret(token != NULL, -IAPAckStatus_EBadParameter);
            pack_accepted(IAPFIDTokenValuesIPodPreferenceAck);
            ack->class_id = token->class_id;
        } break;
        case IAPFIDTokenTypes_EAProtocol: {
            const struct IAPFIDTokenValuesEAProtocolToken* token = iap_span_read(&token_span, sizeof(*token));
            check_ret(token != NULL, -IAPAckStatus_EBadParameter);
            pack_accepted(IAPFIDTokenValuesEAProtocolAck);
            ack->protocol_index = token->protocol_index;
        } break;
        case IAPFIDTokenTypes_BundleSeedIDPref: {
            const struct IAPFIDTokenValuesBundleSeedIDPrefToken* token = iap_span_read(&token_span, sizeof(*token));
            check_ret(token != NULL, -IAPAckStatus_EBadParameter);
            pack_accepted(IAPFIDTokenValuesBundleSeedIDPrefAck);
        } break;
        case IAPFIDTokenTypes_ScreenInfo: {
            const struct IAPFIDTokenValuesScreenInfoToken* token = iap_span_read(&token_span, sizeof(*token));
            check_ret(token != NULL, -IAPAckStatus_EBadParameter);
            pack_accepted(IAPFIDTokenValuesScreenInfoAck);
        } break;
        case IAPFIDTokenTypes_EAProtocolMetadata: {
            const struct IAPFIDTokenValuesEAProtocolMetadataToken* token = iap_span_read(&token_span, sizeof(*token));
            check_ret(token != NULL, -IAPAckStatus_EBadParameter);
            pack_accepted(IAPFIDTokenValuesEAProtocolMetadataAck);
        } break;
        case IAPFIDTokenTypes_AccDigitalAudioSampleRates: {
            const struct IAPFIDTokenValuesAccDigitalAudioSampleRatesToken* token = iap_span_read(&token_span, sizeof(*token));
            check_ret(token != NULL, -IAPAckStatus_EBadParameter);
            while(token_span.size > 0) {
                uint32_t rate;
                check_ret(iap_span_read_32(&token_span, &rate), -IAPAckStatus_EBadParameter);
            }
            pack_accepted(IAPFIDTokenValuesAccDigitalAudioSampleRatesAck);
        } break;
        case IAPFIDTokenTypes_AccDigitalAudioVideoDelay: {
            const struct IAPFIDTokenValuesAccDigitalAudioVideoDelayToken* token = iap_span_read(&token_span, sizeof(*token));
            check_ret(token != NULL, -IAPAckStatus_EBadParameter);
            pack_accepted(IAPFIDTokenValuesAccDigitalAudioVideoDelayAck);
        } break;
        case IAPFIDTokenTypes_MicrophoneCaps: {
            const struct IAPFIDTokenValuesMicrophoneCapsToken* token = iap_span_read(&token_span, sizeof(*token));
            check_ret(token != NULL, -IAPAckStatus_EBadParameter);
            pack_accepted(IAPFIDTokenValuesMicrophoneCapsAck);
        } break;
        default:
            warn("unknown fid %04X", token_header->type << 8 | token_header->subtype);
            return -IAPAckStatus_EBadParameter;
        }
    }
    return 0;
}
