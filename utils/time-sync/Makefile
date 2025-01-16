CFLAGS = -Wall
SRCS = ipod-scsi.c time-sync.c
OBJS = $(patsubst %.c,%.o,$(SRCS))

all: time-sync

clean:
	rm -f ${OBJS} time-sync

%.o: %.c
	gcc ${CFLAGS} -c -o $@ $^

time-sync: ${OBJS}
	gcc ${CFLAGS} -o $@ $^ -lsgutils2

install: time-sync
	cp time-sync /usr/local/bin/
