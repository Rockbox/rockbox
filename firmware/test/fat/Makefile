DRIVERS = ../../drivers

CFLAGS = -g -Wall -DTEST_FAT -I$(DRIVERS) -I.

TARGET = fat

$(TARGET): fat.o ata-sim.o debug.o
	gcc -g -o fat $+ -lfl

fat.o: $(DRIVERS)/fat.c $(DRIVERS)/fat.h $(DRIVERS)/ata.h
	$(CC) $(CFLAGS) -c $< -o $@

ata-sim.o: ata-sim.c $(DRIVERS)/ata.h

debug.o: debug.c debug.h $(DRIVERS)/ata.h

clean:
	rm -f *.o $(TARGET)
	rm -f *~
	rm -f cmd.tab.h lex.yy.c cmd.tab.c
	rm -f core

tar:
	rm -f $(TARGET).tar
	tar cvf $(TARGET).tar -C .. fat
