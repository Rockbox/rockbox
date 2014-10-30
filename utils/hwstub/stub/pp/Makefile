#
# common
#
TOOLCHAIN?=arm-elf-eabi-
CC?=$(TOOLCHAIN)gcc
LD?=$(TOOLCHAIN)gcc
AS?=$(TOOLCHAIN)gcc
OC?=$(TOOLCHAIN)objcopy
DEFINES=
INCLUDES=-I$(CURDIR)
GCCOPTS=-mcpu=arm7tdmi
BUILD_DIR=$(CURDIR)/build/
ROOT_DIR=$(CURDIR)/..

include ../hwstub.make
