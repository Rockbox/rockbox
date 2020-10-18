#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

BUILD_DIR := build
TARGET_DIR := .
DEFINES =
CC ?= gcc
ifndef V
    SILENT := @
endif

CFLAGS := -O3 -g -std=c99 -Wall $(DEFINES) -Isrc/headers
LDFLAGS :=

ifneq ($(ISYSROOT),)
CFLAGS += -isysroot $(ISYSROOT)
endif

SOURCES := \
	src/ciphers/aes/aes.c src/ciphers/aes/aes_tab.c \
	src/misc/crypt/crypt_argchk.c src/misc/crypt/crypt_register_cipher.c src/misc/crypt/crypt_cipher_is_valid.c src/misc/crypt/crypt_cipher_descriptor.c \
	src/misc/zeromem.c src/misc/compare_testvector.c \
	src/modes/cbc/cbc_start.c src/modes/cbc/cbc_decrypt.c src/modes/cbc/cbc_encrypt.c \
	src/hashes/sha1.c


OBJS := $(addprefix $(BUILD_DIR)/,$(patsubst %.c,%.o,$(SOURCES)))

LIB := librbtomcrypt.a

$(LIB): $(OBJS)
	$(info AR $(notdir $@))
	$(SILENT)$(CROSS)$(AR) rcs $(TARGET_DIR)/$@ $^

$(BUILD_DIR)/%.o: %.c
	$(info CC $(notdir $@))
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(CROSS)$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -fr $(BUILDDIR) $(LIB)
