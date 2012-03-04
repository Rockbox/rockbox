#include "loader_strerror.h"

char *loader_strerror(enum error_t error)
{
    switch(error)
    {
    case EOK:
        return "OK";
    case EFILE_NOT_FOUND:
        return "File not found";
    case EREAD_CHKSUM_FAILED:
        return "Read failed (chksum)";
    case EREAD_MODEL_FAILED:
        return "Read failed (model)";
    case EREAD_IMAGE_FAILED:
        return "Read failed (image)";
    case EBAD_CHKSUM:
        return "Bad checksum";
    case EFILE_TOO_BIG:
        return "File too big";
    case EINVALID_FORMAT:
        return "Invalid file format";
#if defined(MI4_FORMAT)
    case EREAD_HEADER_FAILED:
        return "Can't read mi4 header";
    case EKEY_NOT_FOUND:
        return "Can't find crypt key";
    case EDECRYPT_FAILED:
        return "Decryption failed";
#elif defined(RKW_FORMAT)
    case EREAD_HEADER_FAILED:
        return "Can't read RKW header";
    case EINVALID_FORMAT:
        return "Invalid file format";
    case EBAD_HEADER_CHKSUM:
        return "RKW header CRC error";
    case EINVALID_LOAD_ADDR:
        return "RKW Load address mismatch";
    case EBAD_MODEL:
        return "Bad model number";
#endif
    default:
        return "Unknown error";
    }
}
