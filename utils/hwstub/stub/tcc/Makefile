#
# common
#
PREFIX?=arm-elf-eabi-
CC=$(PREFIX)gcc
LD=$(PREFIX)gcc
AS=$(PREFIX)gcc
OC=$(PREFIX)objcopy
DEFINES=
INCLUDES=-I$(CURDIR)
# TCC77x and more recent chips use ARM9E core, beware that TCC760 chip use older cores and are not
# supported for now
GCCOPTS=-mcpu=arm9e
BUILD_DIR=$(CURDIR)/build/
ROOT_DIR=$(CURDIR)/..

include ../hwstub.make
