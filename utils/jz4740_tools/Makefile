WIN_DRIVERS_LIBUSB_DIR = C:\Program Files\LibUSB-Win32
WIN_LIBUSB_INCLUDE_DIR = "$(WIN_DRIVERS_LIBUSB_DIR)\include"
WIN_LIBUSB_LIB_DIR = "$(WIN_DRIVERS_LIBUSB_DIR)\lib\gcc"

linux: usbtool HXFmerge HXFreplace HXFsplit
win: usbtool_win HXFmerge_win HXFsplit_win HXFreplace_win

usbtool:
	gcc -Wall -o usbtool jz4740_usbtool.c -lusb
usbtool_win:
	gcc -Wall -o usbtool.exe jz4740_usbtool.c -lusb -I $(WIN_LIBUSB_INCLUDE_DIR) -L $(WIN_LIBUSB_LIB_DIR)

HXFmerge:
	gcc -Wall -o HXFmerge HXFmerge.c
HXFreplace:
	gcc -Wall -o HXFreplace HXFreplace.c
HXFsplit:
	gcc -Wall -o HXFsplit HXFsplit.c

HXFmerge_win:
	gcc -Wall -o HXFmerge.exe HXFmerge.c
HXFreplace_win:
	gcc -Wall -o HXFreplace.exe HXFreplace.c
HXFsplit_win:
	gcc -Wall -o HXFsplit.exe HXFsplit.c

clean-linux:
	rm HXFmerge HXFreplace HXFsplit usbtool
clean-win:
	del HXFmerge.exe HXFreplace.exe HXFsplit.exe usbtool.exe
