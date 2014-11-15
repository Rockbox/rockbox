#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <setupapi.h>
#include <initguid.h>
#include <ddk/hidsdi.h>
#include <stdint.h>

/* Command Block Descriptor (CDB) */
struct hid_cdb_t
{
    uint8_t command;
    uint32_t length; // big-endian!
    uint8_t reserved[11];
} __attribute__((packed));

// command
#define BLTC_DOWNLOAD_FW    2

/* Command Block Wrapper (CBW) */
struct hid_cbw_t
{
    uint32_t signature; // BLTC or PITC
    uint32_t tag; // returned in CSW
    uint32_t length; // number of bytes to transfer
    uint8_t flags;
    uint8_t reserved[2];
    struct hid_cdb_t cdb;
} __attribute__((packed));

/* HID Command Report */
struct hid_cmd_report_t
{
    uint8_t report_id;
    struct hid_cbw_t cbw;
} __attribute__((packed));

// report id
#define HID_BLTC_DATA_REPORT    2
#define HID_BLTC_CMD_REPORT     1

// signature
#define CBW_BLTC    0x43544C42  /* "BLTC" */
#define CBW_PITC    0x43544950  /* "PITC" */
// flags
#define CBW_DIR_IN  0x80
#define CBW_DIR_OUT 0x00

/* Command Status Wrapper (CSW) */
struct hid_csw_t
{
    uint32_t signature; // BLTS or PITS
    uint32_t tag; // given in CBW
    uint32_t residue; // number of bytes not transferred
    uint8_t status;
} __attribute__((packed));
// signature
#define CSW_BLTS    0x53544C42 /* "BLTS" */
#define CSW_PITS    0x53544950 /* "PITS" */
// status
#define CSW_PASSED      0x00
#define CSW_FAILED      0x01
#define CSW_PHASE_ERROR 0x02

/* HID Status Report */
struct hid_status_report_t
{
    uint8_t report_id;
    struct hid_csw_t csw;
} __attribute__((packed));

#define HID_BLTC_STATUS_REPORT  4

/* HID class */
GUID GUID_DEVINTERFACE_HID;

void init_hid(void)
{
    HidD_GetHidGuid(&GUID_DEVINTERFACE_HID);
}

bool match(USHORT VendorID, USHORT ProductID)
{
    (void) ProductID;
    return VendorID == 0x066f ||
        (VendorID == 0x041e && ProductID == 0x415a);
}

bool probe(const char *path)
{
    HANDLE handle = CreateFileA(path, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if(handle == INVALID_HANDLE_VALUE)
        return false;
    HIDD_ATTRIBUTES attr;
    attr.Size = sizeof(attr);
    bool ret = HidD_GetAttributes(handle, &attr);
    if(ret)
    {
        printf("Potential device: %04x:%04x\n", attr.VendorID, attr.ProductID);
        ret = match(attr.VendorID, attr.ProductID);
    }
    CloseHandle(handle);
    return ret;
}

char *find_device(void)
{
    HDEVINFO dev_info = SetupDiGetClassDevs(&GUID_DEVINTERFACE_HID, NULL, NULL,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if(dev_info == INVALID_HANDLE_VALUE)
    {
        printf("SetupDiGetClassDevs failed: %d\n", GetLastError());
        return NULL;
    }
    /* Initialize the Windows objects. */
    for(DWORD device_index = 0; ; device_index++)
    {
        SP_DEVICE_INTERFACE_DATA devinfo_intf_data;
        devinfo_intf_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        if(!SetupDiEnumDeviceInterfaces(dev_info, NULL, &GUID_DEVINTERFACE_HID,
                device_index, &devinfo_intf_data))
            break;
        DWORD size = 0;
        // ignore return value: it will be FALSE
        SetupDiGetDeviceInterfaceDetailA(dev_info, &devinfo_intf_data,
            NULL, 0, &size, NULL);
        SP_DEVICE_INTERFACE_DETAIL_DATA_A *devinfo_intf_detail = malloc(size);
        devinfo_intf_detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
        if(!SetupDiGetDeviceInterfaceDetailA(dev_info, &devinfo_intf_data,
                devinfo_intf_detail, size, NULL, NULL))
        {
            printf("SetupDiGetDeviceInterfaceDetailA failed2: %d\n", GetLastError());
            goto next;
        }
        SP_DEVINFO_DATA devinfo_data;
        devinfo_data.cbSize = sizeof(SP_DEVINFO_DATA);
        if(!SetupDiEnumDeviceInfo(dev_info, device_index, &devinfo_data))
        {
            printf("SetupDiEnumDeviceInfo failed: %d\n", GetLastError());
            goto next;
        }
        char setup_class[256];
        if(!SetupDiGetDeviceRegistryProperty(dev_info, &devinfo_data, SPDRP_CLASS,
                NULL, (PBYTE)setup_class, sizeof(setup_class), NULL))
        {
            printf("SetupDiGetDeviceRegistryProperty failed: %d\n", GetLastError());
            goto next;
        }
        if(strcmp(setup_class, "HIDClass") != 0)
            goto next;
        if(probe(devinfo_intf_detail->DevicePath))
        {
            char *str = malloc(strlen(devinfo_intf_detail->DevicePath) + 1);
            strcpy(str, devinfo_intf_detail->DevicePath);
            free(devinfo_intf_detail);
            SetupDiDestroyDeviceInfoList(dev_info);
            return str;
        }
        next:
        free(devinfo_intf_detail);
    }
    SetupDiDestroyDeviceInfoList(dev_info);
    return NULL;
}

HANDLE open_hid(const char *path)
{
    return CreateFileA(path, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED, NULL);
}

int get_max_hid_xfer_size(HANDLE handle)
{
    PHIDP_PREPARSED_DATA data;
    if(!HidD_GetPreparsedData(handle, &data))
    {
        printf("HidD_GetPreparsedData failed\n");
        return -1;
    }
    HIDP_CAPS caps;
    if(!HidP_GetCaps(data, &caps))
    {
        printf("HidP_GetCaps failed\n");
        return -1;
    }
    HidD_FreePreparsedData(data);
    // output report length include report ID, so subtract it
    return caps.OutputReportByteLength - 1;
}

static void put32be(uint8_t *buf, uint32_t i)
{
    *buf++ = (i >> 24) & 0xff;
    *buf++ = (i >> 16) & 0xff;
    *buf++ = (i >> 8) & 0xff;
    *buf++ = i & 0xff;
}

struct hid_context_t
{
    OVERLAPPED overlapped;
    DWORD xfered;
    DWORD error;
    HANDLE event;
};

VOID CALLBACK write_hid_completion(DWORD err, DWORD xfered, LPOVERLAPPED overlapped)
{
    struct hid_context_t *ctx = (void *)overlapped->hEvent;
    ctx->error = err;
    ctx->xfered = xfered;
    SetEvent(ctx->event);
}

bool write_hid(HANDLE handle, void *buf, int size)
{
    HANDLE event = CreateEvent(NULL, TRUE, FALSE, NULL);
    struct hid_context_t ctx;
    ctx.overlapped.Offset = 0;
    ctx.overlapped.OffsetHigh = 0;
    ctx.overlapped.hEvent = (HANDLE)&ctx;
    ctx.event = event;
    if(!WriteFileEx(handle, buf, size, &ctx.overlapped, &write_hid_completion))
    {
        CloseHandle(event);
        printf("Write failed: %d\n", GetLastError());
        return false;
    }
    if(WaitForSingleObjectEx(event, INFINITE, TRUE) == WAIT_TIMEOUT)
    {
        CancelIo(handle);
        CloseHandle(event);
        return false;
    }
    CloseHandle(event);
    if(ctx.error != ERROR_SUCCESS)
    {
        printf("An error occured during the transfer: %d\n", ctx.error);
        return false;
    }
    if((int)ctx.xfered != size)
    {
        printf("A short transfer occured\n");
        return false;
    }
    return true;
}

bool send_hid(HANDLE handle, int xfer_size, uint8_t *data, int size, int nr_xfers)
{
    int recv_size;
    uint32_t my_tag = 0xcafebabe;
    uint8_t *xfer_buf = malloc(1 + xfer_size);
    struct hid_cmd_report_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.report_id = HID_BLTC_CMD_REPORT;
    cmd.cbw.signature = CBW_BLTC;
    cmd.cbw.tag = my_tag;
    cmd.cbw.length = size;
    cmd.cbw.flags = CBW_DIR_OUT;
    cmd.cbw.cdb.command = BLTC_DOWNLOAD_FW;
    put32be((void *)&cmd.cbw.cdb.length, size);
    memcpy(xfer_buf, &cmd, sizeof(cmd));

    if(!write_hid(handle, xfer_buf, xfer_size + 1))
    {
        printf("transfer error at init step\n");
        goto Lstatus;
    }

    for(int i = 0; i < nr_xfers; i++)
    {
        xfer_buf[0] = HID_BLTC_DATA_REPORT;
        memcpy(&xfer_buf[1], &data[i * xfer_size], xfer_size);

        if(!write_hid(handle, xfer_buf, xfer_size + 1))
        {
            printf("transfer error at send step %d\n", i);
            goto Lstatus;
        }
    }
    Lstatus:
#if 0
    ret = libusb_interrupt_transfer(dev, 0x81, xfer_buf, xfer_size, 
        &recv_size, 1000);
    if(ret == 0 && recv_size == sizeof(struct hid_status_report_t))
    {
        struct hid_status_report_t *report = (void *)xfer_buf;
        if(report->report_id != HID_BLTC_STATUS_REPORT)
        {
            printf("Error: got non-status report\n");
            return -1;
        }
        if(report->csw.signature != CSW_BLTS)
        {
            printf("Error: status report signature mismatch\n");
            return -2;
        }
        if(report->csw.tag != my_tag)
        {
            printf("Error: status report tag mismtahc\n");
            return -3;
        }
        if(report->csw.residue != 0)
            printf("Warning: %d byte were not transferred\n", report->csw.residue);
        switch(report->csw.status)
        {
            case CSW_PASSED:
                printf("Status: Passed\n");
                return 0;
            case CSW_FAILED:
                printf("Status: Failed\n");
                return -1;
            case CSW_PHASE_ERROR:
                printf("Status: Phase Error\n");
                return -2;
            default:
                printf("Status: Unknown Error\n");
                return -3;
        }
    }
    else if(ret < 0)
        printf("Error: cannot get status report\n");
    else
        printf("Error: status report has wrong size\n");
#endif
    return true;
}

int main(int argc, char **argv)
{
    if(argc != 2)
    {
        printf("usage: %s <file>\n", argv[0]);
        return 1;
    }
    const char *file = argv[1];
    // find device
    init_hid();
    char *path = find_device();
    if(path == NULL)
    {
        printf("Couldn't find a valid device\n");
        return 1;
    }
    printf("Device found at %s\n", path);
    HANDLE handle = open_hid(path);
    if(handle == INVALID_HANDLE_VALUE)
    {
        printf("Cannot open HID device: %d\n", GetLastError());
        goto end;
    }
    // get max transfer size
    int xfer_size = get_max_hid_xfer_size(handle);
    if(xfer_size < 0)
    {
        printf("Cannot get maximum transfer size\n");
        goto end;
    }
    printf("Maximum transfer size: %d\n", xfer_size);
    // open file
    FILE *f = fopen(file, "rb");
    if(f == NULL)
    {
        printf("Cannot open input file\n");
        goto end;
    }
    fseek(f, 0, SEEK_END);
    int file_size = ftell(f);
    int nr_xfers = (file_size + xfer_size - 1) / xfer_size;
    fseek(f, 0, SEEK_SET);
    void *buf = malloc(nr_xfers * xfer_size);
    memset(buf, 0xff, nr_xfers * xfer_size);
    fread(buf, file_size, 1, f);
    fclose(f);
    if(!send_hid(handle, xfer_size, buf, file_size, nr_xfers))
    {
        printf("Cannot upload file to device\n");
        goto end;
    }
    printf("Upload successful\n");
end:
    CloseHandle(handle);
    free(path);
    return 0;
}