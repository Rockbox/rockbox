#include <unistd.h>
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
#include "stage1_upload.h"

/* ### upload sdram init code ### */
int upload_stage1_code(libusb_device_handle *hdev, char *fn_stage1,
                       bool do_scramble)
{
    FILE *f;
    int ret;
    uint8_t *code;
    uint32_t codesize;
    uint16_t cks;

    if ((f = fopen(fn_stage1, "rb")) == NULL)
    {
        fprintf(stderr, "[error]: Could not open file \"%s\"\n", fn_stage1);
        return -10;
    }

    codesize = filesize(f);

    if (codesize > 0x1fe)
    {
        fprintf(stderr, "[error]: Code too big for stage1\n");
        return -11;
    }

    fprintf(stderr, "[stage1]: Loading %d bytes (%s) of code... ", codesize, fn_stage1);

    code = (uint8_t *)malloc(0x200);
    if (code == NULL)
    {
        fprintf(stderr, "\n[error]: Out of memory\n");
        fclose(f);
        return -12;
    }

    memset(code, 0, 0x200);
    if (fread(code, 1, codesize, f) != codesize)
    {
        fprintf(stderr, "\n[error]: I/O error\n");
        fclose(f);
        free(code);
        return -13;
    }

    fprintf(stderr, "done\n");
    fclose(f);

    /* encode data if requested */
    if (do_scramble)
    {

        fprintf(stderr, "[stage1]: Encoding %d bytes of data ... ", codesize);
        scramble(code, code, codesize);
        fprintf(stderr, "done\n");
    }


    fprintf(stderr, "[stage1]: codesize = %d (0x%x)\n", codesize, codesize);

    fprintf(stderr,  "[stage1]: Calculating checksum... ");
    cks = checksum((void *)code, codesize);
    fprintf(stderr, "0x%04x\n", cks);
    code[0x1fe] = (cks >> 8) & 0xff;
    code[0x1ff] = cks & 0xff;
    codesize += 2;

    fprintf(stderr, "[stage1]: Uploading code (%d bytes)... ", codesize);

    ret = libusb_control_transfer(hdev,              /* device handle */
                                  USB_EP0,           /* bmRequestType */
                                  VCMD_UPLOAD,       /* bRequest */
                                  0,                 /* wValue */
                                  VCMD_INDEX_STAGE1, /* wIndex */
                                  code,              /* data */
                                  codesize,          /* wLength */
                                  USB_TIMEOUT        /* timeout */
                                 );
    if (ret < 0)
    {
        fprintf(stderr, "\n[error]: Code upload request failed (ret=%d)\n", ret);
        free(code);
        return -14;
    }

    if (ret != (int)codesize)
    {
        fprintf(stderr, "\n[error]: Sent %d of %d total\n", ret, codesize);
        free(code);
        return -15;
    }

    sleep(1);               /* wait for code to finish */
    fprintf(stderr, "done\n");

    /* free code */
    free(code);

    return 0;
}

