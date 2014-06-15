#
# common
#
CC=arm-elf-eabi-gcc
LD=arm-elf-eabi-gcc
AS=arm-elf-eabi-gcc
OC=arm-elf-eabi-objcopy
DEFINES=
INCLUDES=-I$(CURDIR)
GCCOPTS=-march=armv5te
BUILD_DIR=$(CURDIR)/build/
ROOT_DIR=$(CURDIR)/..

include ../hwstub.make

rkw: $(EXEC_BIN)
	$(SILENT)$(ROOT_DIR)/../../../tools/scramble -rkw -modelnum=73 $(BUILD_DIR)/hwstub.bin $(BUILD_DIR)/hwstub.rkw
