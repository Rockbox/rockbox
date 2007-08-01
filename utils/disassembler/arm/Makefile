CFLAGS = -Wall

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
OUTPUT=arm_disass.exe
CROSS=
CFLAGS+=-mno-cygwin
else
OUTPUT=arm_disass
CROSS=i586-mingw32msvc-
endif

NATIVECC = gcc
CC = $(CROSS)gcc

all: $(OUTPUT)

arm_disass: main.c disasm_arm.c
	gcc $(CFLAGS) -o arm_disass main.c disasm_arm.c
	strip arm_disass

arm_disass.exe: main.c disasm_arm.c
	$(CC) $(CFLAGS) -o arm_disass.exe main.c disasm_arm.c
	$(CROSS)strip arm_disass.exe

clean:
	rm -f arm_disass main.o disasm_arm.o *~
