CC    = sh-elf-gcc 
LD    = sh-elf-ld
AR    = sh-elf-ar
AS    = sh-elf-as
OC    = sh-elf-objcopy

FIRMWARE=../..

INCLUDES=-I$(FIRMWARE) -I$(FIRMWARE)/common -I$(FIRMWARE)/drivers

TARGET = -DARCHOS_PLAYER_OLD=1

CFLAGS = -g -W -Wall -m1 -save-temps -nostdlib -Wstrict-prototypes -fschedule-insns -fno-builtin $(INCLUDES) $(TARGET) -DDEBUG
AFLAGS += -small -relax

SRC := $(wildcard *.c)
OBJS := $(SRC:%.c=%.o) $(FIRMWARE)/crt0.o
DEPS:=.deps
DEPDIRS:=$(DEPS)

all : archos.mod

archos.elf : $(OBJS) app.lds
	$(CC) -nostartfiles -o archos.elf $(OBJS) -lgcc -lrockbox -L$(FIRMWARE) -Tapp.lds -Wl,-Map,archos.map

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
	-rm -f $(OBJS) *.x *.i *.o *.s *.elf *.bin *.map *.mod *.bak *~
	-$(RM) -r $(DEPS)

$(DEPS)/%.d: %.c
	@$(SHELL) -c 'for d in $(DEPDIRS); do { if [ ! -d $$d ]; then mkdir $$d; fi; }; done'
	@echo "Updating dependencies for $<"
	@$(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		|sed '\''s|\($*\)\.o[ :]*|\1.o $(<:%.c=%.d) : |g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

-include $(SRC:%.c=$(DEPS)/%.d)

