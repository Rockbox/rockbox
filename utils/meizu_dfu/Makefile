
all: meizu_dfu

meizu_dfu: meizu_dfu.c
	gcc -o meizu_dfu meizu_dfu.c -lusb

.PHONY: clean
clean:
	rm -f meizu_dfu

