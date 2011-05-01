#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb.h>
#include <stdint.h>

void put32le(uint8_t *buf, uint32_t i)
{
    *buf++ = i & 0xff;
    *buf++ = (i >> 8) & 0xff;
    *buf++ = (i >> 16) & 0xff;
    *buf++ = (i >> 24) & 0xff;
}

void put32be(uint8_t *buf, uint32_t i)
{
    *buf++ = (i >> 24) & 0xff;
    *buf++ = (i >> 16) & 0xff;
    *buf++ = (i >> 8) & 0xff;
    *buf++ = i & 0xff;
}

int main(int argc, char **argv)
{
    int ret;
    uint8_t msg[0x20];
    uint8_t *p;
    FILE *f;
    int i, xfer_size, nr_xfers, recv_size;

    if(argc != 3)
    {
        printf("usage: %s <xfer size> <file>\n", argv[0]);
        return 1;
    }

    char *end;
    xfer_size = strtol(argv[1], &end, 0);
    if(end != (argv[1] + strlen(argv[1])))
    {
        printf("Invalid transfer size !\n");
        return 1;
    }
    
    libusb_device_handle *dev;
    
    libusb_init(NULL);
    
    libusb_set_debug(NULL, 3);
    
    dev = libusb_open_device_with_vid_pid(NULL, 0x066F, 0x3780);
    if(dev == NULL)
    {
        printf("Cannot open device\n");
        return 1;
    }
    
    libusb_detach_kernel_driver(dev, 0);
    libusb_detach_kernel_driver(dev, 4);
    
    libusb_claim_interface (dev, 0);
    libusb_claim_interface (dev, 4);
    
    if (!dev)
    {
        printf("No dev\n");
        exit(1);
    }

    f = fopen(argv[2], "r");
    if(f == NULL)
    {
        perror("cannot open file");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    printf("Transfer size: %d\n", xfer_size);
    nr_xfers = (size + xfer_size - 1) / xfer_size;
    uint8_t *file_buf = malloc(nr_xfers * xfer_size);
    memset(file_buf, 0xff, nr_xfers * xfer_size); // pad with 0xff
    if(fread(file_buf, size, 1, f) != 1)
    {
        perror("read error");
        fclose(f);
        return 1;
    }
    fclose(f);
    
    memset(msg, 0, 0x20);
    
    p = msg;
    
    *p++ = 0x01; // Init upload command
    *p++ = 'B'; // Signature
    *p++ = 'L';
    *p++ = 'T';
    *p++ = 'C';
    put32le(p, 0x1); // I guess version or sub-command
    p += 4;
    put32le(p, size); // Payload size
    
    // The second command starts at 0x20
    
    p = &msg[0x10];
    
    *p++ = 0x02; // Start upload
    put32be(p, size); // Payload size, again
    
    ret = libusb_control_transfer(dev, 
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 0x9, 0x201, 0,
        msg, 0x20, 1000);
    if(ret < 0)
    {
        printf("transfer error at init step\n");
        return 1;
    }

    uint8_t *xfer_buf = malloc(1 + xfer_size);

    for(i = 0; i < nr_xfers; i++)
    {
        xfer_buf[0] = 0x2;
        memcpy(&xfer_buf[1], &file_buf[i * xfer_size], xfer_size);
        
        ret = libusb_control_transfer(dev, 
            LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 
            0x9, 0x202, 0, xfer_buf, xfer_size + 1, 1000);
        if(ret < 0)
        {
            printf("transfer error at send step %d\n", i);
            return 1;
        }
    }

    ret = libusb_interrupt_transfer(dev, 0x81, xfer_buf, xfer_size, &recv_size,
        1000);
    if(ret < 0)
    {
        printf("transfer error at final stage\n");
        return 1;
    }
    
    printf("ret %i\n", ret);
    
    return 0;
}

