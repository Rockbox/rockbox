R ?= ../../..
T = $(R)/firmware/target/arm/s5l8702/ipodnano3g
CFLAGS = -O2 -g -std=gnu99 -Wall -Wextra -Wno-unused-parameter -Istubs -I$(T) \
         -include ftl_hooks.h
DUMP ?=

# The test hooks are appended to the FTL source, each group only if that
# tree does not already define it.
HOOKFLAGS = $(shell grep -q 'int ftl_sb_locate' $(FTL) || echo -DNEED_HOOKS_LOCATE) \
            $(shell grep -q 'uint32_t ftl_pool_snapshot' $(FTL) || echo -DNEED_HOOKS_WEAR)

FTL = $(T)/ftl-nano3g.c
HDRS = $(T)/ftl-target.h $(T)/nand-target.h mock_nand.h ftl_hooks.h ftl_hooks.c

ftl_built.c: $(FTL) ftl_hooks.c FORCE
	cat $(FTL) ftl_hooks.c > $@
FORCE:

all: test_ftl test_mock test_write test_wear test_badblock test_eligible test_restore \
     test_geometry test_format test_permute bench_write

test_ftl: ftl_built.c mock_nand.c test_ftl.c $(HDRS)
	$(CC) $(CFLAGS) $(HOOKFLAGS) -o $@ ftl_built.c mock_nand.c test_ftl.c

test_mock: mock_nand.c test_mock.c $(HDRS)
	$(CC) $(CFLAGS) -o $@ mock_nand.c test_mock.c

test_write: ftl_built.c mock_nand.c test_write.c $(HDRS)
	$(CC) $(CFLAGS) $(HOOKFLAGS) -o $@ ftl_built.c mock_nand.c test_write.c

test_wear: ftl_built.c mock_nand.c test_wear.c $(HDRS)
	$(CC) $(CFLAGS) $(HOOKFLAGS) -o $@ ftl_built.c mock_nand.c test_wear.c

test_badblock: ftl_built.c mock_nand.c test_badblock.c $(HDRS)
	$(CC) $(CFLAGS) $(HOOKFLAGS) -o $@ ftl_built.c mock_nand.c test_badblock.c

test_eligible: ftl_built.c mock_nand.c test_eligible.c $(HDRS)
	$(CC) $(CFLAGS) $(HOOKFLAGS) -o $@ ftl_built.c mock_nand.c test_eligible.c

test_restore: ftl_built.c mock_nand.c test_restore.c $(HDRS)
	$(CC) $(CFLAGS) $(HOOKFLAGS) -o $@ ftl_built.c mock_nand.c test_restore.c

test_permute: mock_nand.c test_permute.c $(HDRS)
	$(CC) $(CFLAGS) -o $@ mock_nand.c test_permute.c

test_geometry: ftl_built.c mock_nand.c test_geometry.c $(HDRS)
	$(CC) $(CFLAGS) $(HOOKFLAGS) -o $@ ftl_built.c mock_nand.c test_geometry.c

test_format: ftl_built.c mock_nand.c test_format.c $(HDRS)
	$(CC) $(CFLAGS) $(HOOKFLAGS) -o $@ ftl_built.c mock_nand.c test_format.c

replay: ftl_built.c mock_nand.c replay.c $(HDRS)
	$(CC) $(CFLAGS) $(HOOKFLAGS) -o $@ ftl_built.c mock_nand.c replay.c

test_crash: ftl_built.c mock_nand.c test_crash.c $(HDRS)
	$(CC) $(CFLAGS) $(HOOKFLAGS) -o $@ ftl_built.c mock_nand.c test_crash.c

bench_write: ftl_built.c mock_nand.c bench_write.c $(HDRS)
	$(CC) $(CFLAGS) $(HOOKFLAGS) -o $@ ftl_built.c mock_nand.c bench_write.c

check: all
	test -n "$(DUMP)"
	./test_mock $(DUMP)
	./test_ftl $(DUMP)
	./test_permute $(DUMP)
	./test_ftl halves:$(DUMP)
	./test_ftl both:$(DUMP)
	./test_write $(DUMP)
	./test_wear $(DUMP)
	./test_badblock $(DUMP)
	./test_eligible $(DUMP)
	./test_restore $(DUMP)
	./test_geometry $(DUMP)
	./test_format blank:2k4
	./test_format blank:2k8g
	./test_format blank:4k2
	./test_format blank:halves
	./test_format blank:both
	./test_format blank:single4k
	./test_format blank:split13

# Power-cut campaigns on formatted blank media, 4KiB pages included
check-crash-blank: test_crash
	for g in 2k4 4k2 halves both single4k split13; do for s in 1 2; do \
	    ./test_crash blank:$$g 300 $$s 6000 0 || exit 1; \
	    ./test_crash blank:$$g 300 $$s 6000 1 || exit 1; \
	done; done

# Power-cut campaigns on the dump laid out as modes 9 and 4
check-crash-layouts: test_crash
	test -n "$(DUMP)"
	for g in halves both; do for s in 1 2 3 4; do \
	    ./test_crash $$g:$(DUMP) 300 $$s 6000 0 || exit 1; \
	    ./test_crash $$g:$(DUMP) 300 $$s 6000 1 || exit 1; \
	done; done

check-crash: test_crash
	test -n "$(DUMP)"
	for s in $$(seq 1 24); do \
	    ./test_crash $(DUMP) 300 $$s 6000 0; \
	    ./test_crash $(DUMP) 300 $$s 6000 1; \
	done

clean:
	rm -f ftl_built.c test_ftl test_mock test_write test_wear test_badblock \
	      test_eligible test_restore test_geometry test_format test_permute \
	      test_crash replay bench_write

.PHONY: all check check-crash check-crash-blank check-crash-layouts clean FORCE
