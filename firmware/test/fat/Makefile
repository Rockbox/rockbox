FIRMWARE = ../..
DRIVERS = ../../drivers

INCLUDE = -I$(DRIVERS) -I$(FIRMWARE) -I$(FIRMWARE)/common
RINCLUDE =  -I$(FIRMWARE)/include 
DEFINES =  -DTEST_FAT -DDEBUG -DCRT_DISPLAY -DDISK_WRITE

CFLAGS = -g -Wall $(DEFINES) -I. $(INCLUDE) $(RINCLUDE) -DLITTLE_ENDIAN
SIMFLAGS = -g -Wall $(DEFINES) -I. $(INCLUDE)

TARGET = fat

$(TARGET): fat.o ata-sim.o main.o disk.o debug.o dir.o file.o ctype.o
	gcc -g -o fat $+ -lfl

fat.o: $(DRIVERS)/fat.c $(DRIVERS)/fat.h $(DRIVERS)/ata.h
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

ata-sim.o: ata-sim.c $(DRIVERS)/ata.h
	$(CC) $(SIMFLAGS) -DSIMULATOR -c $< -o $@

main.o: main.c $(DRIVERS)/ata.h
	$(CC) $(SIMFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET)
	rm -f *~
	rm -f cmd.tab.h lex.yy.c cmd.tab.c
	rm -f core

tar:
	rm -f $(TARGET).tar
	tar cvf $(TARGET).tar -C .. fat
