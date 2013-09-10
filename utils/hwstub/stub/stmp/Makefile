#
# common
#
CC=arm-elf-eabi-gcc
LD=arm-elf-eabi-gcc
AS=arm-elf-eabi-gcc
OC=arm-elf-eabi-objcopy
SBTOELF=$(CURDIR)/../../../imxtools/sbtools/elftosb
DEFINES=
INCLUDES=-I$(CURDIR)
GCCOPTS=-mcpu=arm926ej-s
BUILD_DIR=$(CURDIR)/build/
ROOT_DIR=$(CURDIR)/..
EXEC=$(BUILD_DIR)/hwstub.sb

include ../hwstub.make

$(BUILD_DIR)/hwstub.sb: $(EXEC_BIN)
	$(call PRINTS,SBTOELF $(@F))
	$(SILENT)$(SBTOELF) -z -c hwstub.db -o $@ $<