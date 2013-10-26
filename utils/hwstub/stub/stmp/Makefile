#
# common
#
CC=arm-elf-eabi-gcc
LD=arm-elf-eabi-gcc
AS=arm-elf-eabi-gcc
OC=arm-elf-eabi-objcopy
SBTOOLS=$(CURDIR)/../../../imxtools/sbtools
ELFTOSB=$(SBTOOLS)/elftosb
ELFTOSB1=$(SBTOOLS)/elftosb1
DEFINES=
INCLUDES=-I$(CURDIR)
GCCOPTS=-mcpu=arm926ej-s
BUILD_DIR=$(CURDIR)/build/
ROOT_DIR=$(CURDIR)/..
EXEC=$(BUILD_DIR)/hwstub.sb $(BUILD_DIR)/hwstub.sb1

include ../hwstub.make

$(BUILD_DIR)/hwstub.sb: $(EXEC_BIN)
	$(call PRINTS,ELFTOSB $(@F))
	$(SILENT)$(ELFTOSB) -z -c hwstub.db -o $@ $<


$(BUILD_DIR)/hwstub.sb1: $(EXEC_BIN)
	$(call PRINTS,ELFTOSB1 $(@F))
	$(SILENT)$(ELFTOSB1) -load-binary $< 0 -jump 0 -o $@