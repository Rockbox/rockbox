CC    = sh-elf-gcc 
LD    = sh-elf-ld
AR    = sh-elf-ar
AS    = sh-elf-as
OC    = sh-elf-objcopy

INCLUDES=-I../../ -I../../drivers

# Pick a target to build for
#TARGET = -DARCHOS_PLAYER=1
TARGET = -DARCHOS_PLAYER_OLD=1
#TARGET = -DARCHOS_RECORDER=1

CFLAGS = -g -O -Wall -m1 -save-temps -nostdlib -Wstrict-prototypes -fomit-frame-pointer -fschedule-insns -fno-builtin $(INCLUDES) $(TARGET)
AFLAGS += -small -relax

OBJS= ../../crt0.o ../../system.o main.o timer.o \
	../../thread.o ../../kernel.o ../../debug.o

%.o: %.S
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) $(DEFS) -c $<


all : archos.mod

archos.elf : $(OBJS) app.lds
	$(CC) -nostartfiles -o archos.elf $(OBJS) -lgcc -Tapp.lds -Wl,-Map,archos.map

archos.bin : archos.elf
	$(OC) -O binary archos.elf archos.bin

archos.asm: archos.bin
	sh2d -sh1 archos.bin > archos.asm

archos.mod : archos.bin
	scramble archos.bin archos.mod

archos.mod.gz : archos.mod
	gzip -f archos.mod

dist:
	tar czvf dist.tar.gz Makefile main.c start.s app.lds

clean:
	-rm -f $(OBJS) *.s *.x *.i *.o *.elf *.bin *.map *.mod *.bak *~

install:
	mount /mnt/archos; cp archos.mod /mnt/archos; umount /mnt/archos
