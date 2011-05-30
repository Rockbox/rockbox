/* on ubuntu compile with gcc -W rkusbtool.c -o rkusbtool -lusb-1.0 -I/usr/include/libusb-1.0/ */
#include <libusb.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define VERSION "v0.1"

#define RETRY_MAX 5
#define USB_TIMEOUT 512
#define VENDORID  0x071b
#define PRODUCTID 0x3203

#define OUT_EP  0x01
#define IN_EP   0x82

#define CBW_SIGNATURE 0x43425355
#define CSW_SIGNATURE 0x53425355
#define SCSICMD_READ_12 0xa8

/* rockchip specific commands */
#define RK_CMD            0xe0
#define RK_GET_VERSION    0xffffffff
#define RK_SWITCH_ROCKUSB 0xfeffffff
#define RK_CHECK_USB      0xfdffffff
#define RK_OPEN_SYSDISK   0xfcffffff

enum {
      NONE = 0,
      INFO = 1,
      RKUSB = 2,
      SYSDISK = 4,
      CHECKUSB = 8
};

enum {
      COMMAND_PASSED = 0,
      COMMAND_FAILED = 1,
      PHASE_ERROR    = 2
};

struct CBWCB_t
{
    uint8_t  cbCode;
    uint8_t  cbLun;
    uint32_t LBA;
    uint32_t cbLen;
    uint8_t  reseved;
    uint8_t  control;
} __attribute__((__packed__));

struct CBW_t
{
    uint32_t dCBWSignature;
    uint32_t dCBWTag;
    uint32_t dCBWDataTransferLength;
    uint8_t  bmCBWFlags;
    uint8_t  bCBWLUN;
    uint8_t  bCBWCBLength;
    uint8_t  CBWCB[16];
} __attribute__((__packed__));

struct CSW_t
{
    uint32_t dCSWSignature;
    uint32_t dCSWTag;
    uint32_t dCSWDataResidue;
    uint8_t  bCSWStatus;
} __attribute__((__packed__));

static int send_msc_cmd(libusb_device_handle *hdev, struct CBWCB_t *cbwcb, uint32_t data_len, uint32_t *reftag)
{
    struct CBW_t cbw;
    int ret, repeat, transferred;
    static uint32_t tag = 0xdaefbc01;

    memset(&cbw, 0, sizeof(cbw));
    cbw.dCBWSignature = CBW_SIGNATURE;
    cbw.dCBWTag = tag++;
    cbw.dCBWDataTransferLength = data_len;
    cbw.bmCBWFlags = 0x80; /* device to host */
    cbw.bCBWLUN = 0;
    cbw.bCBWCBLength = sizeof(struct CBWCB_t);
    memcpy(cbw.CBWCB, cbwcb, sizeof(struct CBWCB_t));

    *reftag = cbw.dCBWTag;
    do
    {
        /* transfer command to the device */
        ret = libusb_bulk_transfer(hdev, OUT_EP, (unsigned char*)&cbw, 31, &transferred, USB_TIMEOUT);
        if (ret == LIBUSB_ERROR_PIPE)
        {
            libusb_clear_halt(hdev, OUT_EP);
        }
        repeat++;
    } while ((ret == LIBUSB_ERROR_PIPE) && (repeat < RETRY_MAX));

    if (ret != LIBUSB_SUCCESS)
    {
        printf("error: command transfer error\n");
        return -1;
    }

    return 0;
}

static int get_msc_csw(libusb_device_handle *hdev, uint32_t reftag)
{
    struct CSW_t csw;
    int ret, repeat, transferred;

    /* get CSW response from device */
    repeat = 0;
    do
    {
        ret = libusb_bulk_transfer(hdev, IN_EP, (unsigned char *)&csw, 13, &transferred, USB_TIMEOUT);
        if (ret == LIBUSB_ERROR_PIPE)
        {
            libusb_clear_halt(hdev, IN_EP);
        }
        repeat++;
    } while ((ret == LIBUSB_ERROR_PIPE) && (repeat < RETRY_MAX));

    if (ret != LIBUSB_SUCCESS)
    {
        printf("error reading CSW\n");
        return -3;
    }

    if (transferred != 13)
    {
        printf("error wrong size of CSW packet\n");
        return -4;
    }

    if (csw.dCSWSignature != CSW_SIGNATURE)
    {
        printf("error: wrong CSW signature.\n");
        return -5;
    }

    if (csw.dCSWTag != reftag)
    {
        printf("error: CSW dCSWTag mismatch\n");
        return -6;
    }

    if (csw.bCSWStatus)
    {
        /* In case of CSW indicating error dump the content of the packet */
        printf ("dCSWSignature: 0x%0x\n", csw.dCSWSignature);
        printf ("dCSWTag: 0x%0x\n", csw.dCSWTag);
        printf ("dCSWDataResidue: 0x%0x\n", csw.dCSWDataResidue);
        printf ("bCSWStatus: 0x%0x\n", csw.bCSWStatus);
    }

    return csw.bCSWStatus;
}

static int rk_cmd(libusb_device_handle *hdev, uint32_t command, uint8_t *buf, uint8_t len)
{
    struct CBWCB_t cbwcb;
    int ret, transferred;
    uint32_t reftag;

    /* enter command */
    memset(&cbwcb, 0, sizeof(cbwcb));
    cbwcb.cbCode = SCSICMD_READ_12;
    cbwcb.cbLun = RK_CMD;
    cbwcb.LBA = command; /* RK_GET_VERSION, RK_OPEN_SYSDISK, RK_SWITCH_ROCKUSB */
    cbwcb.cbLen = len;   /* size of transfer in response to this command */

    ret = send_msc_cmd(hdev, &cbwcb, len, &reftag);

    /* get the response */
    if (len > 0)
    {
        ret = libusb_bulk_transfer(hdev, IN_EP, buf, len, &transferred, USB_TIMEOUT);
        if (ret != LIBUSB_SUCCESS || transferred != len)
        {
            printf("error: reading response data failed\n");
            return -2;
        }
    }

    return get_msc_csw(hdev, reftag);
}

static int get_sense(libusb_device_handle *hdev)
{
    struct CBWCB_t cbwcb;
    unsigned char sense[0x12];
    int size, ret;
    uint32_t reftag;

    memset(&cbwcb, 0, sizeof(cbwcb));
    cbwcb.cbCode = 0x03;
    cbwcb.cbLun = 0;
    cbwcb.LBA = 0;
    cbwcb.cbLen = 0x12;

    ret = send_msc_cmd(hdev, &cbwcb, 0x12, &reftag);
    libusb_bulk_transfer(hdev, IN_EP, (unsigned char*)&sense, 0x12, &size, USB_TIMEOUT);

    return get_msc_csw(hdev, reftag);
}

static void usage(void)
{
    printf("Usage: rkusbtool [options]\n");
    printf("-h|--help         This help message\n");
    printf("-i|--info         Get version string from the device\n");
    printf("-d|--dfu          Put device into DFU mode\n");
    printf("-s|--sysdisk      Open system disk\n");
    printf("-c|--checkusb     Check if dev is in System or Loader USB mode\n");
}

int main (int argc, char **argv)
{
    libusb_device_handle *hdev;
    int ret;
    int i = 0, action = NONE;
    uint32_t ver[3];

    if (argc < 2)
    {
        usage();
        return 1;
    }
    
    /* print banner */
    fprintf(stderr,"rkusbtool " VERSION "\n");
    fprintf(stderr,"(C) Marcin Bukat 2011\n");
    fprintf(stderr,"This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stderr,"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");

    /* arguments handling */
    while (i < argc)
    {
        if ((strcmp(argv[i],"-i")==0) || (strcmp(argv[i],"--info")==0))
        {
            action |= INFO;
        }
        else if ((strcmp(argv[i],"-d")==0) || (strcmp(argv[i],"--dfu")==0))
        {
            action |= RKUSB;
        }
        else if ((strcmp(argv[i],"-s")==0) || (strcmp(argv[i],"--sysdisk")==0))
        {
            action |= SYSDISK;
        }
        else if ((strcmp(argv[i],"-c")==0) || (strcmp(argv[i],"--checkusb")==0))
        {
            action |= CHECKUSB;
        }
        else if ((strcmp(argv[i],"-h")==0) || (strcmp(argv[i],"--help")==0))
        {
            usage();
            return 0;
        }
        i++;
    }

    /* initialize libusb */
    libusb_init(NULL);
    /* usb_set_debug(2); */

    hdev = libusb_open_device_with_vid_pid(NULL, VENDORID, PRODUCTID);
    if (hdev == NULL)
    {
        printf("error: can't open device\n");
        return -10;
    }

    ret = libusb_kernel_driver_active(hdev, 0);

    if (ret < 0)
    {
        printf ("error checking kernel driver active\n");
        libusb_close(hdev);
        return -3;
    }
    else
    {
        if (ret)
            libusb_detach_kernel_driver(hdev, 0);
    }

    ret = libusb_set_configuration(hdev, 1);
    if (ret < 0)
    {
        printf("error: could not select configuration (1)\n");
        libusb_close(hdev);
        return -3;
    }

    ret = libusb_claim_interface(hdev, 0);
    if (ret < 0)
    {
            printf("error: could not claim interface #0\n");
            libusb_close(hdev);
            return -11;
    }

    ret = libusb_set_interface_alt_setting(hdev, 0, 0);
    if ( ret != LIBUSB_SUCCESS)
    {
        printf("error: could not set alt setting for interface #0\n");
        libusb_close(hdev);
        return -11;
    }
    
    /* BulkOnly reset */
    //ret = libusb_control_transfer(hdev, 0x21, 0xff, 0, 0, NULL, 0, USB_TIMEOUT);

   /* BulkOnly get max lun */
   //ret = libusb_control_transfer(hdev, 0xa1, 0xfe, 0, 0, &maxlun, 1, USB_TIMEOUT);

    /* Devices that do not support multiple LUNs may STALL this command. */
   //if (ret == 0)
   //   maxlun = -1;
 
    //printf("MAXLUN: %d\n", maxlun);

    get_sense(hdev);

    if (action & INFO)
    {
        ret = rk_cmd(hdev, RK_GET_VERSION, (uint8_t *)ver, 12);

        if (ret)
        {
            printf("error sending RK_GET_VERSION command. Err 0x%0x\n", ret);
            libusb_close(hdev);
            return ret;
        }

        printf("Rockchip device info:\n");
        printf("loader ver: %x.%x\n", (ver[0]>>16)&0xff, ver[0]&0xff);
        printf("kernel ver: %x.%x\n", (ver[1]>>16)&0xff, ver[1]&0xff);
        printf("sdk ver:    %x.%x\n", (ver[2]>>16)&0xff, ver[2]&0xff);
    }

    if (action & CHECKUSB)
    {
        printf("Checking USB mode...\n");
        ret = rk_cmd(hdev, RK_CHECK_USB, (uint8_t *)ver, 1);

        //if (ret)
        //{
        //    libusb_close(hdev);
        //    return ret;
        //}

        if (*(char *)ver)
            printf("The device is in Loader USB mode\n");
        else
            printf("The device is in System USB mode\n");
    }

    if (action & SYSDISK)
    {
        printf("Opening system disk...\n");
        ret = rk_cmd(hdev, RK_OPEN_SYSDISK, NULL, 0);

        if (ret)
        {
            libusb_close(hdev);
            return ret;
        }
    }

    if (action & RKUSB)
    {
        printf("Switching into rk DFU mode...\n");
        ret = rk_cmd(hdev, RK_SWITCH_ROCKUSB, NULL, 0);

        if (ret)
        {
            libusb_close(hdev);
            return ret;
        }
    }

    libusb_close(hdev);
    libusb_exit(NULL);
    return 0;
}
