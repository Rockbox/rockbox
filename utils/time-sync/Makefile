CFLAGS = -Wall
SRCS = ipod-scsi.c time-sync.c
OBJS = $(patsubst %.c,%.o,$(SRCS))

all: time-sync

clean:
	rm -f ${OBJS} ipod-time-sync

%.o: %.c
	gcc ${CFLAGS} -c -o $@ $^

time-sync: ${OBJS}
	gcc ${CFLAGS} -lsgutils2 -o $@ $^

install: time-sync
	cp time-sync /usr/local/bin/