
.PHONY: all clean
PROGS = extract_section
CC = gcc
CFLAGS = -O1 -g -W -Wall


all: $(PROGS)
	$(MAKE) -C cramfs-1.1

clean:
	$(MAKE) -C cramfs-1.1 clean
	rm -f extract_section
