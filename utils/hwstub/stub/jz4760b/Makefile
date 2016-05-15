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

include ../hwstub.make
