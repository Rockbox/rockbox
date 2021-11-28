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
	   test/main.o \
	   test/test_flashmap.o \
	   test/test_stream.o
TBIN = xf_test

# dependency needs to be built manually
MTARINC = -I../microtar/src
MTARLIB = ../microtar/libmicrotar.a

CPPFLAGS = -Iinclude -Itest_lib $(MTARINC) -D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=200809L
CFLAGS = -std=c99 -Wall -Wextra
LDFLAGS =

PROFRAW_FILE=$(TBIN).profraw
PROFDATA_FILE=$(TBIN).profdata

export LLVM_PROFILE_FILE=$(PROFRAW_FILE)

ifeq ($(COVERAGE),1)
  CC = clang
  CFLAGS += -g -Og -fprofile-instr-generate -fcoverage-mapping
  LDFLAGS += -fprofile-instr-generate -fcoverage-mapping
else
  CFLAGS += -O2
endif

ifeq ($(SANITIZE),1)
  CFLAGS += -fsanitize=address -fsanitize=undefined
  LDFLAGS += -fsanitize=address -fsanitize=undefined
endif

.PHONY: all
all: $(LIB) $(TBIN)

.PHONY: test
test: $(TBIN)
	@./$(TBIN)

.PHONY: cov
cov: $(PROFDATA_FILE)
	@llvm-cov report $(TBIN) -instr-profile=$(PROFDATA_FILE)

.PHONY: cov-show
cov-show: $(PROFDATA_FILE)
	@llvm-cov show $(TBIN) --use-color -instr-profile=$(PROFDATA_FILE) $(f) | less -R

.PHONY: clean
clean:
	rm -f $(LIB) $(OBJ)
	rm -f $(TBIN) $(TOBJ)
	rm -f $(PROFRAW_FILE) $(PROFDATA_FILE)
	rm -f fakeNAND.bin fakeNAND_meta.bin

$(LIB): $(OBJ)
	$(AR) rcs $@ $^ >/dev/null

$(TBIN): $(TOBJ) $(LIB) $(MTARLIB)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# use separate rule instead of depending on 'test' to avoid re-running
$(PROFRAW_FILE): $(TBIN)
	@./$(TBIN)

$(PROFDATA_FILE): $(PROFRAW_FILE)
	@llvm-profdata merge -sparse $(PROFRAW_FILE) -o $(PROFDATA_FILE)
