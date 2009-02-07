CFLAGS = -Wall
LIBS = -lmtp
OUTPUT = sendfirm

all: $(OUTPUT)

$(OUTPUT): sendfirm.c
	gcc $(CFLAGS) $(LIBS) -o $(OUTPUT) sendfirm.c || gcc $(CFLAGS) $(LIBS) -DOLDMTP -o $(OUTPUT) sendfirm.c

$(OUTPUT).exe: sendfirm_win.c
	mingw32-gcc $(CFLAGS) -o $(OUTPUT).exe sendfirm_win.c MTP_DLL.dll

clean:
	rm -f $(OUTPUT)
