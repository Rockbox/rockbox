CFLAGS = -Wall
LIBS = -lmtp
OUTPUT = sendfirm
EXTRA_CFLAGS = $(shell echo -e \
	'\#include <libmtp.h>\nlibmtp version: LIBMTP_VERSION' | \
	gcc -E -P - -o - | grep -q '^libmtp version: 0\.2' && echo '-DOLDMTP')

all: $(OUTPUT)

$(OUTPUT): sendfirm.c
	gcc $(EXTRA_CFLAGS) $(CFLAGS) $(LIBS) -o $(OUTPUT) sendfirm.c

$(OUTPUT).exe: sendfirm_win.c
	mingw32-gcc $(CFLAGS) -o $(OUTPUT).exe sendfirm_win.c MTP_DLL.dll

clean:
	rm -f $(OUTPUT)
