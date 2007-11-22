CFLAGS = -Wall
LIBS = -lmtp
OUTPUT = sendfirm

all: $(OUTPUT)

$(OUTPUT): sendfirm.c
	gcc $(CFLAGS) $(LIBS) -o $(OUTPUT) sendfirm.c

clean:
	rm -f $(OUTPUT)
