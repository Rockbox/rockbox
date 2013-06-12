PREFIX?=arm-elf-eabi-
CC=$(PREFIX)gcc
LD=$(PREFIX)gcc
AS=$(PREFIX)gcc
OC=$(PREFIX)objcopy
SBTOOLS=../../sbtools/
CFLAGS=-W -Wall -Wundef -O -nostdlib -ffreestanding -Wstrict-prototypes -pipe -std=gnu99 -mcpu=arm926ej-s -fomit-frame-pointer -Wno-pointer-sign -Wno-override-init -ffunction-sections
CFLAGS_3700=$(CFLAGS) -DHAVE_STMP3700
ASFLAGS=$(CFLAGS) -D__ASSEMBLER__
ASFLAGS_3700=$(CFLAGS_3700) -D__ASSEMBLER__
OCFLAGS=
LINKER_FILE=hwemul.lds
LDFLAGS=-lgcc -Os -nostdlib -Tlink.lds -Wl,-Map,hwemul.map
LDFLAGS_3700=-lgcc -Os -nostdlib -Tlink.lds -Wl,-Map,hwemul3700.map
SRC_C=$(wildcard *.c)
SRC_S=$(wildcard *.S)
OBJ_C=$(SRC_C:.c=.o)
OBJ_S=$(SRC_S:.S=.o)
OBJ_C_3700=$(SRC_C:.c=.3700.o)
OBJ_S_3700=$(SRC_S:.S=.3700.o)
OBJ=$(OBJ_C) $(OBJ_S)
OBJ_3700=$(OBJ_C_3700) $(OBJ_S_3700)
OBJ_EXCEPT_CRT0=$(filter-out crt0.o,$(OBJ))
OBJ_EXCEPT_CRT0_3700=$(filter-out crt0.3700.o,$(OBJ_3700))
DEPS=$(OBJ:.o=.d)
EXEC_ELF=hwemul.elf
EXEC_SB=hwemul.sb
EXEC_ELF_3700=hwemul3700.elf
EXEC_SB_3700=hwemul3700.sb

ELF2SB=$(SBTOOLS)/elftosb -d
ELF2SB_CMD=-c hwemul.db
ELF2SB_KEY=-z
SBLOADER=$(SBTOOLS)/sbloader
SBLOADER_CMD=0 $(EXEC_SB)
SBLOADER_CMD_3700=0 $(EXEC_SB_3700)

TOOLS=../../../../tools/
SCRAMBLE=$(TOOLS)/scramble

EXEC=$(EXEC_SB) $(EXEC_SB_3700) $(EXEC_ELF) $(EXEC_ELF_3700)

all: $(EXEC)

# pull in dependency info for *existing* .o files
-include $(DEPS)

%.3700.o: %.c
	$(CC) $(CFLAGS_3700) -c -o $@ $<
	$(CC) -MM $(CFLAGS_3700) $*.c > $*.d
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
	$(CC) -MM $(CFLAGS) $*.c > $*.d
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

%.3700.o: %.S
	$(AS) $(ASFLAGS_3700) -c -o $@ $<

%.o: %.S
	$(AS) $(ASFLAGS) -c -o $@ $<

link.lds: $(LINKER_FILE)
	$(CC) -E -x c - < $< | sed '/#/d' > $@
	
$(EXEC_ELF): $(OBJ) link.lds
	$(LD) $(LDFLAGS) -o $@ $(OBJ_EXCEPT_CRT0)

$(EXEC_SB): $(EXEC_ELF)
	$(ELF2SB) $(ELF2SB_CMD) $(ELF2SB_KEY) -o $@

$(EXEC_ELF_3700): $(OBJ_3700) link.lds
	$(LD) $(LDFLAGS_3700) -o $@ $(OBJ_EXCEPT_CRT0_3700)

$(EXEC_SB_3700): $(EXEC_ELF_3700)
	$(ELF2SB) $(ELF2SB_CMD) $(ELF2SB_KEY) -o $@

sbload: $(EXEC_SB)
	$(SBLOADER) $(SBLOADER_CMD)

sbload3700: $(EXEC_SB_3700)
	$(SBLOADER) $(SBLOADER_CMD_3700)

clean:
	rm -rf $(OBJ) $(OBJ_3700) $(DEPS) $(EXEC) *.map
