#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# Collect dummy C files in firmware/asm
ASM_DUMMY_SRC := $(notdir $(call preprocess, $(FIRMDIR)/asm/SOURCES))

# Get the corresponding real source files under firmware/asm/$ARCH (C or ASM)
# strip arch_ prefix from $(ARCH)
ASM_ARCH := $(subst arch_,,$(ARCH))
ASM_C_SRC := $(addprefix $(FIRMDIR)/asm/$(ASM_ARCH)/,$(ASM_DUMMY_SRC))
ASM_S_SRC := $(ASM_C_SRC:.c=.S)

# ASM_SRC now contains only files that exist under $ARCH
ASM_SRC := $(wildcard $(ASM_C_SRC))
ASM_SRC += $(wildcard $(ASM_S_SRC))

# GEN_SRC now contains a .c for each file in ASM_DUMMY_SRC that's not in ASM_SRC
# I.e. fallback to a generic C source if no corresponding file in $ARCH is found
GEN_SRC := $(filter-out $(notdir $(ASM_SRC:.S=.c)),$(ASM_DUMMY_SRC))
GEN_SRC := $(addprefix $(FIRMDIR)/asm/,$(GEN_SRC))

FIRMLIB_SRC += $(ASM_SRC)
FIRMLIB_SRC += $(GEN_SRC)
