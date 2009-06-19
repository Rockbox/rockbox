WIN_DRIVERS_LIBUSB_DIR = C:\Program Files\LibUSB-Win32
WIN_LIBUSB_INCLUDE_DIR = "$(WIN_DRIVERS_LIBUSB_DIR)\include"
WIN_LIBUSB_LIB_DIR = "$(WIN_DRIVERS_LIBUSB_DIR)\lib\gcc"

CFLAGS=-Wall
CC=gcc

linux: usbtool HXFmerge HXFreplace HXFsplit IHFSsplit HXF2IHFS DLanalyser
win: usbtool_win HXFmerge_win HXFsplit_win HXFreplace_win IHFSsplit_win HXF2IHFS_win DLanalyser_win

bin2c: ../../rbutil/tools/bin2c.c
	$(CC) $(CFLAGS) -o bin2c ../../rbutil/tools/bin2c.c

bin2c.exe: ../../rbutil/tools/bin2c.c
	$(CC) $(CFLAGS) -o bin2c.exe ../../rbutil/tools/bin2c.c

jz_xloader.c: jz_xloader.bin
	./bin2c jz_xloader.bin jz_xloader

usbtool: jz4740_usbtool.c bin2c jz_xloader.c
	$(CC) $(CFLAGS) -o usbtool jz4740_usbtool.c jz_xloader.c -lusb
usbtool_win: jz4740_usbtool.c bin2c.exe jz_xloader.c
	$(CC) $(CFLAGS) -o usbtool.exe jz4740_usbtool.c jz_xloader.c -lusb -I $(WIN_LIBUSB_INCLUDE_DIR) -L $(WIN_LIBUSB_LIB_DIR)

HXFmerge: HXFmerge.c
	$(CC) $(CFLAGS) -o HXFmerge HXFmerge.c
HXFreplace: HXFreplace.c
	$(CC) $(CFLAGS) -o HXFreplace HXFreplace.c
HXFsplit: HXFsplit.c
	$(CC) $(CFLAGS) -o HXFsplit HXFsplit.c
IHFSsplit: IHFSsplit.c
	$(CC) $(CFLAGS) -o IHFSsplit IHFSsplit.c
HXF2IHFS: HXF2IHFS.c
	$(CC) $(CFLAGS) -o HXF2IHFS HXF2IHFS.c
DLanalyser: DLanalyser.c
	$(CC) $(CFLAGS) -o DLanalyser DLanalyser.c

HXFmerge_win: HXFmerge.c
	$(CC) $(CFLAGS) -o HXFmerge.exe HXFmerge.c
HXFreplace_win: HXFreplace.c
	$(CC) $(CFLAGS) -o HXFreplace.exe HXFreplace.c
HXFsplit_win: HXFsplit.c
	$(CC) $(CFLAGS) -o HXFsplit.exe HXFsplit.c
IHFSsplit_win: IHFSsplit.c
	$(CC) $(CFLAGS) -o IHFSsplit.exe IHFSsplit.c
HXF2IHFS_win: HXF2IHFS.c
	$(CC) $(CFLAGS) -o HXF2IHFS.exe HXF2IHFS.c
DLanalyser_win: DLanalyser.c
	$(CC) $(CFLAGS) -o DLanalyser.exe DLanalyser.c

clean-linux:
	rm HXFmerge HXFreplace HXFsplit usbtool IHFSsplit HXF2IHFS DLanalyser bin2c
clean-win:
	del HXFmerge.exe HXFreplace.exe HXFsplit.exe usbtool.exe IHFSsplit.exe HXF2IHFS.exe DLanalyser.exe bin2c.exe
