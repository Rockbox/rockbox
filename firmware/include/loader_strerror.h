enum error_t {
    EFILE_EMPTY = 0,
    EFILE_NOT_FOUND = -1,
    EREAD_CHKSUM_FAILED = -2,
    EREAD_MODEL_FAILED = -3,
    EREAD_IMAGE_FAILED = -4,
    EBAD_CHKSUM = -5,
    EFILE_TOO_BIG = -6,
    EINVALID_FORMAT = -7,
    EKEY_NOT_FOUND = -8,
    EDECRYPT_FAILED = -9,
    EREAD_HEADER_FAILED = -10,
    EBAD_HEADER_CHKSUM = -11,
    EINVALID_LOAD_ADDR = -12,
    EBAD_MODEL = -13
};

char *loader_strerror(enum error_t error);
