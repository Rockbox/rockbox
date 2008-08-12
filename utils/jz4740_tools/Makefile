WIN_DRIVERS_LIBUSB_DIR = C:\Program Files\LibUSB-Win32
WIN_LIBUSB_INCLUDE_DIR = "$(WIN_DRIVERS_LIBUSB_DIR)\include"
WIN_LIBUSB_LIB_DIR = "$(WIN_DRIVERS_LIBUSB_DIR)\lib\gcc"

CFLAGS=-Wall

linux: usbtool HXFmerge HXFreplace HXFsplit IHFSsplit HXF2IHFS
win: usbtool_win HXFmerge_win HXFsplit_win HXFreplace_win IHFSsplit_win HXF2IHFS_win

usbtool:
	$(CC) $(CFLAGS) -o usbtool jz4740_usbtool.c -lusb
usbtool_win:
	$(CC) $(CFLAGS) -o usbtool.exe jz4740_usbtool.c -lusb -I $(WIN_LIBUSB_INCLUDE_DIR) -L $(WIN_LIBUSB_LIB_DIR)

HXFmerge:
	$(CC) $(CFLAGS) -o HXFmerge HXFmerge.c
HXFreplace:
	$(CC) $(CFLAGS) -o HXFreplace HXFreplace.c
HXFsplit:
	$(CC) $(CFLAGS) -o HXFsplit HXFsplit.c
IHFSsplit:
	$(CC) $(CFLAGS) -o IHFSsplit IHFSsplit.c
HXF2IHFS:
	$(CC) $(CFLAGS) -o HXF2IHFS HXF2IHFS.c

HXFmerge_win:
	$(CC) $(CFLAGS) -o HXFmerge.exe HXFmerge.c
HXFreplace_win:
	$(CC) $(CFLAGS) -o HXFreplace.exe HXFreplace.c
HXFsplit_win:
	$(CC) $(CFLAGS) -o HXFsplit.exe HXFsplit.c
IHFSsplit_win:
	$(CC) $(CFLAGS) -o IHFSsplit.exe IHFSsplit.c
HXF2IHFS_win:
	$(CC) $(CFLAGS) -o HXF2IHFS.exe HXF2IHFS.c

clean-linux:
	rm HXFmerge HXFreplace HXFsplit usbtool IHFSsplit HXF2IHFS
clean-win:
	del HXFmerge.exe HXFreplace.exe HXFsplit.exe usbtool.exe IHFSsplit.exe HXF2IHFS.exe
