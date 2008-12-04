FIRMWARE = ../..

export BUILDDATE=$(shell date -u +'-DYEAR=%Y -DMONTH=%m -DDAY=%d')
export CPU=arm
export TARGET=-DIPOD_VIDEO
export TARGET_INC=-I$(FIRMWARE)/target/arm/ipod/video -I$(FIRMWARE)/target/arm/ipod -I$(FIRMWARE)/target/arm

DRIVERS = ../../drivers
EXPORT = ../../export

INCLUDE = -I$(EXPORT)

RINCLUDE =  -I$(FIRMWARE)/include 
DEFINES =  -DTEST_FAT -DDEBUG -DCRT_DISPLAY -DDISK_WRITE -DHAVE_FAT16SUPPORT

CFLAGS = -g -Wall $(DEFINES) -I. $(INCLUDE) $(RINCLUDE) $(BUILDDATE) 
SIMFLAGS = -g -Wall $(DEFINES) -I. $(INCLUDE)

TARGET = fat


$(TARGET): fat.o ata-sim.o main.o disk.o debug.o dir.o file.o ctype.o
	gcc -g -o fat $+ -lfl

fat.o: $(DRIVERS)/fat.c $(EXPORT)/fat.h $(EXPORT)/ata.h
	$(CC) $(CFLAGS) -DSIMULATOR -c $< -o $@

ctype.o: $(FIRMWARE)/common/ctype.c
	$(CC) $(CFLAGS) -c $< -o $@

disk.o: $(FIRMWARE)/common/disk.c
	$(CC) $(CFLAGS) -c $< -o $@

dir.o: $(FIRMWARE)/common/dir.c
	$(CC) $(CFLAGS) -c $< -o $@

file.o: $(FIRMWARE)/common/file.c
	$(CC) $(CFLAGS) -c $< -o $@

debug.o: $(FIRMWARE)/debug.c
	$(CC) $(SIMFLAGS) -DSIMULATOR -c $< -o $@

ata-sim.o: ata-sim.c $(EXPORT)/ata.h
	$(CC) $(SIMFLAGS) -DSIMULATOR -c $< -o $@

dir.h: $(FIRMWARE)/include/dir.h
	ln -s $(FIRMWARE)/include/dir.h .

file.h: $(FIRMWARE)/include/file.h
	ln -s $(FIRMWARE)/include/file.h .

main.o: main.c $(EXPORT)/ata.h dir.h file.h
	$(CC) $(SIMFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET)
	rm -f *~
	rm -f cmd.tab.h lex.yy.c cmd.tab.c
	rm -f core
	rm -f dir.h file.h

tar:
	rm -f $(TARGET).tar
	tar cvf $(TARGET).tar -C .. fat
