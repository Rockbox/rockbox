#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <libusb.h>

#include "rk27load.h"
#include "common.h"
#include "scramble.h"
#include "checksum.h"
#include "stage2_upload.h"

int upload_stage2_code(libusb_device_handle *hdev, char *fn_stage2,
                       bool do_scramble)
{
    FILE *f;
    uint32_t codesize;
    uint8_t *code;
    uint16_t cks;
    int ret;

    if ((f = fopen(fn_stage2, "rb")) == NULL)
    {
        fprintf(stderr, "[error]: Could not open file \"%s\"\n", fn_stage2);
        return -21;
    }

    codesize = filesize(f);

    fprintf(stderr, "[stage1]: Loading %d bytes (%s) of code... ", codesize, fn_stage2);

    code = (uint8_t *) malloc(codesize + 0x400);
    if (code == NULL)
    {
        fprintf(stderr, "\n[error]: Out of memory\n");
        fclose(f);
        return -22;

    }

    memset(code, 0, codesize + 0x400);

    if (fread(code, 1, codesize, f) != codesize)
    {
        fprintf(stderr, "\n[error]: I/O error\n");
        fclose(f);
        free(code);
        return -23;
    }
    fprintf(stderr, "done\n");
    fclose(f);

    codesize = ((codesize + 0x201) & 0xfffffe00) - 2;

    if (do_scramble)
    {
        /* encode data if its user code */
        fprintf(stderr, "[stage2]: Encoding %d bytes data... ", codesize);
        scramble(code, code, codesize);
        fprintf(stderr, "done\n");
    }

    fprintf(stderr, "[stage2]: Calculating checksum... ");
    cks = checksum(code, codesize);
    code[codesize + 0] = (cks >> 8) & 0xff;
    code[codesize + 1] = cks & 0xff;
    codesize += 2;
    fprintf(stderr, "0x%04x\n", cks);

    fprintf(stderr, "[stage2]: Uploading code (%d bytes)... ", codesize);

    ret = libusb_control_transfer(hdev,              /* device handle */
                                  USB_EP0,           /* bmRequestType */
                                  VCMD_UPLOAD,       /* bRequest */
                                  0,                 /* wValue */
                                  VCMD_INDEX_STAGE2, /* wIndex */
                                  code,              /* data */
                                  codesize,          /* wLength */
                                  USB_TIMEOUT        /* timeout */
                                 );

    if (ret < 0)
    {
        fprintf(stderr, "\n[error]: Code upload request failed (ret=%d)\n", ret);
        free(code);
        return -24;
    }

    if (ret != (int)codesize)
    {
        fprintf(stderr, "[error]: Sent %d of %d total\n", ret, codesize);
        free(code);
        return -25;
    }

    fprintf(stderr, "done\n");

    free(code);
    return 0;
}

