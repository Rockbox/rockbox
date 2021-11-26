CPPFLAGS = -Isrc
CFLAGS = -std=c99 -Wall -Wextra -O2

MTAR_OBJ = mtar.o
MTAR_BIN = mtar

MICROTAR_OBJ = src/microtar.o src/microtar-stdio.o
MICROTAR_LIB = libmicrotar.a

$(MTAR_BIN): $(MTAR_OBJ) $(MICROTAR_LIB)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^

$(MICROTAR_LIB): $(MICROTAR_OBJ)
	$(AR) cr $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

src/microtar.o: src/microtar.h
src/microtar-stdio.o: src/microtar.h src/microtar-stdio.h
mtar.o: src/microtar.h src/microtar-stdio.h

clean:
	rm -f $(MICROTAR_LIB) $(MICROTAR_OBJ)
	rm -f $(MTAR_BIN) $(MTAR_OBJ)
