OBJ = src/xf_error.o \
	  src/xf_flashmap.o \
	  src/xf_nandio.o \
	  src/xf_package.o \
	  src/xf_stream.o \
	  src/xf_update.o
LIB = libx1000-installer.a

TOBJ = test_lib/core_alloc.o \
	   test_lib/fakenand.o \
	   test_lib/file.o \
	   test_lib/pathfuncs.o \
	   test_lib/md5.o \
	   test_lib/strlcpy.o \
	   test/main.o
TBIN = xf_test

# dependency needs to be built manually
MTARINC = -I../microtar/src
MTARLIB = ../microtar/libmicrotar.a

CPPFLAGS = -Iinclude -Itest_lib $(MTARINC) -D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=200809L
CFLAGS = -std=c99 -Wall -Wextra -O2

all: $(LIB) $(TBIN)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(LIB): $(OBJ)
	$(AR) rcs $@ $^ >/dev/null

$(TBIN): $(TOBJ) $(LIB) $(MTARLIB)
	$(CC) -o $@ $^

clean:
	rm -f $(LIB) $(OBJ)
	rm -f $(TBIN) $(TOBJ)
