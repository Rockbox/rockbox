FIRMWARE = ../..

DRIVERS = ../../drivers
EXPORT = ../../export

BUILDDATE=$(shell date -u +'-DYEAR=%Y -DMONTH=%m -DDAY=%d')
INCLUDE = -I$(EXPORT) -I$(FIRMWARE)/include -I$(FIRMWARE)/target/hosted -I$(FIRMWARE)/target/hosted/sdl
DEFINES =  -DTEST_FAT -DDEBUG -DDISK_WRITE -DHAVE_FAT16SUPPORT -D__PCTOOL__

CFLAGS = -g -Wall -std=gnu99 -Wno-pointer-sign $(DEFINES) $(BUILDDATE) -I. $(INCLUDE) -I$(FIRMWARE)/libc/include
SIMFLAGS = -g -Wall -std=gnu99 -Wno-pointer-sign $(DEFINES) -I. $(INCLUDE)

TARGET = fat

all: $(TARGET)

$(TARGET): fat.o ata-sim.o main.o disk.o dir.o file.o ctype.o unicode.o strlcpy.o
	gcc -g -o fat $+

fat.o: $(DRIVERS)/fat.c $(EXPORT)/fat.h $(EXPORT)/ata.h
	$(CC) $(CFLAGS) -c $< -o $@

ctype.o: $(FIRMWARE)/libc/ctype.c
	$(CC) $(CFLAGS) -c $< -o $@

disk.o: $(FIRMWARE)/common/disk.c
	$(CC) $(CFLAGS) -c $< -o $@

dir.o: $(FIRMWARE)/common/dir_uncached.c
	$(CC) $(CFLAGS) -c $< -o $@

file.o: $(FIRMWARE)/common/file.c
	$(CC) $(CFLAGS) -c $< -o $@

unicode.o: $(FIRMWARE)/common/unicode.c
	$(CC) $(CFLAGS) -c $< -o $@

strlcpy.o: $(FIRMWARE)/common/strlcpy.c
	$(CC) $(CFLAGS) -c $< -o $@

ata-sim.o: ata-sim.c $(EXPORT)/ata.h
	$(CC) $(SIMFLAGS) -c $< -o $@

main.o: main.c $(EXPORT)/ata.h
	$(CC) $(SIMFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET)
