all: rkwtool

rkwtool: rkw.c main.c
	gcc -g -std=c99 -o $@ -W -Wall $^

clean:
	rm -fr rkwtool
