HPATH = /usr/local/sh-gcc/bin
CC    = $(HPATH)/sh-elf-gcc 
LD    = $(HPATH)/sh-elf-ld
AR    = $(HPATH)/sh-elf-ar
AS    = $(HPATH)/sh-elf-as
OC    = $(HPATH)/sh-elf-objcopy

INCLUDES=-I.

CFLAGS = -Os -Wall -m1 -nostdlib -Wstrict-prototypes -fomit-frame-pointer -fschedule-insns $(INCLUDES)
AFLAGS += -small -relax

OBJS= start.o main.o

%.o: %.s
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) $(DEFS) -c $<


all : archos.mod # archos.asm 

main.o: main.c

archos.elf : $(OBJS) app.lds
	$(CC) -nostdlib -o archos.elf $(OBJS) -lgcc -Tapp.lds -Wl,-Map,archos.map

archos.bin : archos.elf
	$(OC) -O binary archos.elf archos.bin

archos.asm: archos.bin
	sh2d -sh1 archos.bin > archos.asm

archos.mod : archos.bin
	scramble archos.bin archos.mod

dist:
	tar czvf dist.tar.gz Makefile main.c start.s app.lds

clean:
	-rm -f *.x *.i *.o *.elf *.bin *.map *.mod *.bak *~

