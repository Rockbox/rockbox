CC    = sh-elf-gcc 
LD    = sh-elf-ld
AR    = sh-elf-ar
AS    = sh-elf-as
OC    = sh-elf-objcopy

INCLUDES=-I../../common -I../.. -I../../drivers

TARGET = -DARCHOS_PLAYER_OLD=1

CFLAGS = -g -Wall -m1 -save-temps -nostdlib -Wstrict-prototypes -fschedule-insns -fno-builtin $(INCLUDES) $(TARGET) -DDEBUG
AFLAGS += -small -relax

OBJS= ../../crt0.o main.o ../../drivers/i2c.o ../../drivers/mas.o \
	../../debug.o ../../kernel.o thread.o ../../common/sprintf.o \
	../../panic.o ../../system.o ../../drivers/led.o \
	../../drivers/lcd.o ata.o ../../drivers/fat.o \
	../../common/disk.o ../../common/file.o ../../common/dir.o

%.o: %.S
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) $(DEFS) -c $<


all : archos.mod

main.o: main.c

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
	-rm -f $(OBJS) *.x *.i *.o *.elf *.bin *.map *.mod *.bak *~

install:
	mount /mnt/archos; cp archos.mod /mnt/archos; umount /mnt/archos

thread.o: ../../thread.c
	$(CC) -O $(CFLAGS) -fomit-frame-pointer -c $<

ata.o: ../../drivers/ata.c
	$(CC) -O $(CFLAGS) -c $<

main.o: main.c
	$(CC) -O $(CFLAGS) -c $<

