CFLAGS = -Wall
LIBS = -lmtp
OUTPUT = sendfirm
EXTRA_CFLAGS = $(shell printf \
	'\#include <libmtp.h>\nlibmtp version: LIBMTP_VERSION\n' | \
	gcc -E -P - -o - | grep -q '^libmtp version: 0\.2' && echo '-DOLDMTP')

all: $(OUTPUT)

$(OUTPUT): sendfirm.c
	gcc $(EXTRA_CFLAGS) $(CFLAGS) -o $(OUTPUT) sendfirm.c $(LIBS)

$(OUTPUT).exe: sendfirm_win.c
	i686-w64-mingw32-gcc $(CFLAGS) -o $(OUTPUT).exe sendfirm_win.c MTP_DLL.dll

clean:
	rm -f $(OUTPUT) $(OUTPUT).exe
