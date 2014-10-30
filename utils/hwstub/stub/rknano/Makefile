#
# common
#
TOOLCHAIN?=arm-unknown-eabi-
CC=$(TOOLCHAIN)gcc
LD=$(TOOLCHAIN)gcc
AS=$(TOOLCHAIN)gcc
OC=$(TOOLCHAIN)objcopy
DEFINES=
INCLUDES=-I$(CURDIR)
GCCOPTS=-mthumb -mcpu=cortex-m3
BUILD_DIR=$(CURDIR)/build/
ROOT_DIR=$(CURDIR)/..

include ../hwstub.make
