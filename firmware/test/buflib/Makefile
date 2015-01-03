FIRMWARE=../..

CC ?= gcc
CFLAGS += -g -O2 -DDEBUG -D__PCTOOL__ -std=gnu99 -I$(FIRMWARE)/include -I$(FIRMWARE)/export -I.
LDFLAGS += -L. -lpthread

.PHONY: clean all

TARGETS_OBJ = test_main.o
TARGETS = $(TARGETS_OBJ:.o=)

LIB_OBJ = 	buflib.o \
			crc32.o \
			strlcpy.o
LIB_FILE = libbuflib.a
LIB = buflib


ifndef V
SILENT:=@
else
VERBOSEOPT:=-v
endif

PRINTS=$(SILENT)$(call info,$(1))

all: $(TARGETS)

test_%: test_%.o $(LIB_FILE)
	$(call PRINTS,LD $@)$(CC) $(LDFLAGS) -o $@ $< -l$(LIB)

$(TARGETS): $(TARGETS_OBJ) $(LIB_FILE)

buflib.o: $(FIRMWARE)/buflib.c
	$(CC) $(CFLAGS) -c $< -o $@

crc32.o: $(FIRMWARE)/common/crc32.c
	$(CC) $(CFLAGS) -c $< -o $@

strlcpy.o: $(FIRMWARE)/common/strlcpy.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(call PRINTS,CC $<)$(CC) $(CFLAGS) -c $<

$(LIB_FILE): $(LIB_OBJ)
	$(call PRINTS,AR $@)ar rcs $@ $^

clean:
	rm *.o $(TARGETS) $(LIB_FILE)
