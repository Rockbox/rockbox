
all: meizu_dfu

meizu_dfu: meizu_dfu.c
	gcc -I/usr/local/include -L/usr/local/lib -o meizu_dfu meizu_dfu.c -lusb

.PHONY: clean
clean:
	rm -f meizu_dfu

