#
# common
#
CC=arm-elf-eabi-gcc
LD=arm-elf-eabi-gcc
AS=arm-elf-eabi-gcc
OC=arm-elf-eabi-objcopy
CFLAGS=-W -Wall -Wundef -O -nostdlib -ffreestanding -Wstrict-prototypes -pipe -std=gnu99 -mcpu=arm926ej-s -fomit-frame-pointer -Wno-pointer-sign -Wno-override-init -ffunction-sections
ASFLAGS=$(CFLAGS) -D__ASSEMBLER__
OCFLAGS=
LINKER_FILE=hwstub.lds
TMP_LDS=link.lds
TMP_MAP=hwstub.map
LDFLAGS=-lgcc -Os -nostdlib -T$(TMP_LDS) -Wl,-Map,$(TMP_MAP)
SRC_C=$(wildcard *.c)
SRC_S=$(wildcard *.S)
OBJ_C=$(SRC_C:.c=.o)
OBJ_S=$(SRC_S:.S=.o)
OBJ=$(OBJ_C) $(OBJ_S)
OBJ_EXCEPT_CRT0=$(filter-out crt0.o,$(OBJ))
DEPS=$(OBJ:.o=.d)
EXEC_ELF=hwstub.elf

#
# image production
#
TOOLS=../../../tools
SBTOOLS=../../imxtools/sbtools

# sb (stmp37xx)
EXEC_SB=hwstub.sb
ELF2SB=$(SBTOOLS)/elftosb -d
ELF2SB_CMD=-c hwstub.db
ELF2SB_KEY?=-z

# sb1 (stmp36xx)
EXEC_SB1=hwstub.sb1
ELF2SB1_CMD=-loadjump $(EXEC_ELF)
ELF2SB1_KEY?=
ELF2SB1=$(SBTOOLS)/elftosb1 -d

EXEC=$(EXEC_ELF) $(EXEC_SB) $(EXEC_SB1)

all: $(EXEC)

# pull in dependency info for *existing* .o files
-include $(DEPS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
	$(CC) -MM $(CFLAGS) $*.c > $*.d
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

%.o: %.S
	$(AS) $(ASFLAGS) -c -o $@ $<

$(TMP_LDS): $(LINKER_FILE)
	$(CC) -E -x c - < $< | sed '/#/d' > $@

$(EXEC_ELF): $(OBJ) $(TMP_LDS)
	$(LD) $(LDFLAGS) -o $@ $(OBJ_EXCEPT_CRT0)

$(EXEC_SB): $(EXEC_ELF)
	$(ELF2SB) $(ELF2SB_CMD) $(ELF2SB_KEY) -o $@

$(EXEC_SB1): $(EXEC_ELF)
	$(ELF2SB1) $(ELF2SB1_CMD) $(ELF2SB1_KEY) -o $@

clean:
	rm -rf $(OBJ) $(DEPS) $(EXEC) $(TMP_LDS) $(TMP_MAP)