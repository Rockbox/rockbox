FIRMWARE=../..

CC ?= gcc
# Note: Don't be fooled by MEMORYSIZE here
# We have a fixed, predictable buffer in UT_core_allocator_init()
CFLAGS += -g -O2 -DDEBUG -D__PCTOOL__ -DBUFLIB_UNIT_TEST -DMEMORYSIZE=8 -DBUFLIB_DEBUG_BLOCKS -std=gnu99 -I$(FIRMWARE)/include -I$(FIRMWARE)/export -I.
LDFLAGS += -L. -lpthread

.PHONY: clean all

TARGETS_OBJ = test_main.o \
			  test_main2.o \
			  test_move.o \
			  test_move2.o \
			  test_max.o \
			  test_shrink.o \
			  test_shrink_unaligned.o \
			  test_shrink_startchanged.o \
			  test_shrink_cb.o

TARGETS = $(TARGETS_OBJ:.o=)

LIB_OBJ = 	buflib.o \
			core_alloc.o \
			crc32.o \
			strlcpy.o \
			util.o

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

core_alloc.o: $(FIRMWARE)/core_alloc.c
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
