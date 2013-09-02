all: nandextract

nandextract: nandextract.c libbch.c
	gcc -g -std=c99 -o $@ -W -Wall $^

clean:
	rm -fr nandextract
