OBJECTS = main.o db.o array_buffer.o unique.o malloc.o \
	header.o artist.o album.o song.o file.o \
	tag_dummy.o

all : tagdb parser

tagdb : $(OBJECTS)
	$(CC) -o tagdb $(OBJECTS)

parser: parser.o malloc.o
	$(CC) -o parser parser.o malloc.o
	
main.o : main.c config.h

db.o : db.c db.h config.h

array_buffer.o : array_buffer.c array_buffer.h config.h
unique.o : unique.c unique.h
malloc.o : malloc.c malloc.h config.h

header.o : header.c header.h config.h
artist.o : artist.c artist.h config.h
album.o : album.c album.h config.h
song.o : song.c song.h config.h
file.o : file.c file.h config.h

tag_dummy.o : tag_dummy.c tag_dummy.h config.h

parser.o : parser.c config.h

clean :
	rm -rf *.o tagdb parser
