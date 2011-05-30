#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <libusb.h>

#include "rk27load.h"
#include "common.h"
#include "scramble.h"
#include "checksum.h"
#include "stage3_upload.h"

int upload_stage3_code(libusb_device_handle *hdev, char *fn_stage3)
{
    FILE *f;
    uint32_t codesize;
    uint32_t remain;
    uint8_t *code;
    uint16_t send_size = 0x200;
    uint32_t i = 0;
    int ret, transfered;

    if ((f = fopen(fn_stage3, "rb")) == NULL)
    {
        fprintf(stderr, "[error]: Could not open file \"%s\"\n", fn_stage3);
        return -31;
    }

    codesize = filesize(f);

    fprintf(stderr, "[stage3]: Loading user code (%d bytes)... ", codesize);

    /* allocate buffer */
    code = (uint8_t *) malloc(codesize + 0x204);
    if (code == NULL)
    {
        fprintf(stderr, "\n[error]: Out of memory\n");
        fclose(f);
        return -32;
    }

    memset(code, 0, codesize + 0x204);
    /* read usercode into buffer */
    if (fread(&code[4], 1, codesize, f) != codesize)
    {
        fprintf(stderr, "\n[error]: I/O error\n");
        fclose(f);
        free(f);
        return -33;
    }
    fprintf(stderr, "done\n");

    fclose(f);

    /* put code size at the first 4 bytes */
    codesize += 4;
    code[0] = codesize & 0xff;
    code[1] = (codesize >> 8) & 0xff;
    code[2] = (codesize >> 16) & 0xff;
    code[3] = (codesize >> 24) & 0xff;

    fprintf(stderr, "[stage3]: Uploading user code (%d bytes)... ", codesize);

    remain = codesize;

    while (remain > 0)
    {
        if (remain < 0x200)
            send_size = remain;

        ret = libusb_bulk_transfer(hdev,                           /* handle */
                                   1,                              /* EP */
                                   &code[i * 0x200],               /* data */
                                   send_size,                      /* length */
                                   &transfered,                    /* xfered */
                                   USB_TIMEOUT                     /* timeout */
                                  );

        if (ret != LIBUSB_SUCCESS)
        {
            fprintf(stderr, "\n[error]: Bulk transfer error (%d, %d)\n", ret, i);
            free(code);
            return -34;
        }

        remain -= send_size;
        i++;
    }

    fprintf(stderr,"done (sent %d blocks)\n", i);
    return 0;
}

