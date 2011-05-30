#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <libusb.h>

#include "rk27load.h"
#include "common.h"
#include "stage1_upload.h"
#include "stage2_upload.h"
#include "stage3_upload.h"

#define VERSION "v0.2"

enum {
      NONE = 0,
      ENCODE_S1 = 1,
      ENCODE_S2 = 2
};

static void usage(char *name)
{
    printf("usage: (sudo) %s [-e1 -e2] -s1 stage1.bin -s2 stage2.bin -s3 usercode.bin\n", name);
    printf("stage1.bin - binary of the stage1 (sdram init)\n");
    printf("stage2.bin - binary of the stage2 bootloader\n");
    printf("usercode.bin - binary of the custom usercode\n");
    printf("\n");
    printf("options:\n");
    printf("-e1 - encode stage1 bootloader\n");
    printf("-e2 - encode stage2 bootloader\n");    
}

int main(int argc, char **argv)
{
    libusb_device_handle *hdev;
    char *filenames[3];
    int i=1, action=0, ret=0;

    while (i < argc)
    {
        if (strcmp(argv[i],"-e1") == 0)
        {
            action |= ENCODE_S1;
            i++;
        }
        else if (strcmp(argv[i],"-e2") == 0)
        {
            action |= ENCODE_S2;
            i++;
        }
        else if (strcmp(argv[i],"-s1") == 0)
        {
            i++;
            if (i == argc)
            {
                usage(argv[0]);
                return -1;
            }
            filenames[0] = argv[i];
            printf("%s", argv[i]);
            i++;
        }
        else if (strcmp(argv[i],"-s2") == 0)
        {
            i++;
            if (i == argc)
            {
                usage(argv[0]);
                return -2;
            }
            filenames[1] = argv[i];
            i++;
        }
        else if (strcmp(argv[i],"-s3") == 0)
        {
            i++;
            if (i == argc)
            {
                usage(argv[0]);
                return -3;
            }
            filenames[2] = argv[i];
            i++;
        }
        else
        {
            usage(argv[0]);
            return -4;
        }
    }

 
    fprintf(stderr,"rk27load " VERSION "\n");
    fprintf(stderr,"(C) Marcin Bukat 2011\n");
    fprintf(stderr,"Based on rk27load ver. 0.1 written by AleMaxx (alemaxx at hotmail.de)\n\n");
    fprintf(stderr,"This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stderr,"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");

    /* initialize libusb */
    libusb_init(NULL);

    /* configure device */
    fprintf(stderr, "[info]: Initializing device... ");
    hdev = libusb_open_device_with_vid_pid(NULL, VENDORID, PRODUCTID);

    if (hdev == NULL)
    {
        fprintf(stderr, "\n[error]: Could not find rockchip device\n");
        ret = -2;
        goto finish;
    }

    ret = libusb_set_configuration(hdev, 1);
    if (ret < 0)
    {
        fprintf(stderr, "\n[error]: Could not select configuration (1)\n");
        ret = -3;
        goto finish;
    }

    ret = libusb_claim_interface(hdev, 0);
    if (ret < 0)
    {
        fprintf(stderr, "\n[error]: Could not claim interface #0\n");
        ret = -4;
        goto finish;
    }

    ret = libusb_set_interface_alt_setting(hdev, 0, 0);
    if (ret < 0)
    {
        fprintf(stderr, "\n[error]: Could not set alternate interface #0\n");
        ret = -5;
        goto finish;
    }

    fprintf(stderr, "done\n");


    ret = upload_stage1_code(hdev, filenames[0], (action & ENCODE_S1));
    if (ret < 0)
        goto finish;

    ret = upload_stage2_code(hdev, filenames[1], (action & ENCODE_S2));
    if (ret < 0)
        goto finish;

    ret = upload_stage3_code(hdev, filenames[2]);
    if (ret < 0)
        goto finish;

    /* done */
    ret = 0;

  finish:
    if (hdev != NULL)
        libusb_close(hdev);

    if (ret < 0)
        fprintf(stderr, "[error]: Error %d\n", ret);

    return ret;
}
