#
# common
#
PREFIX?=mipsel-elf-
CC=$(PREFIX)gcc
LD=$(PREFIX)gcc
AS=$(PREFIX)gcc
OC=$(PREFIX)objcopy
DEFINES=
INCLUDES=-I$(CURDIR)
GCCOPTS=-march=mips32 -G0
BUILD_DIR=$(CURDIR)/build/
ROOT_DIR=$(CURDIR)/..
JZ4760TOOLS=$(CURDIR)/../../../jz4760_tools
PACKTOOLS=$(JZ4760TOOLS)/packtools
EXEC=$(BUILD_DIR)/factory.x1

include ../hwstub.make

$(BUILD_DIR)/factory.x1: $(EXEC_BIN)
	$(call PRINTS,PACKTOOLS $(@F))
	$(SILENT)$(PACKTOOLS) --scramble -o $@ -i $<
