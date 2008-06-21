WIN_DRIVERS_LIBUSB_DIR = D:\Program Files\LibUSB-Win32
WIN_LIBUSB_INCLUDE_DIR = "$(WIN_DRIVERS_LIBUSB_DIR)\include"
WIN_LIBUSB_LIB_DIR = "$(WIN_DRIVERS_LIBUSB_DIR)\lib\gcc"

usbtool:
	gcc -Wall -o usbtool jz4740_usbtool.c -lusb

usbtoolwin:
	gcc -Wall -o usbtool.exe jz4740_usbtool.c -lusb -I $(WIN_LIBUSB_INCLUDE_DIR) -L $(WIN_LIBUSB_LIB_DIR)