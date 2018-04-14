CC=gcc
CFLAGS=-Wall

SOURCE1=./master.c
SOURCE2=./slave.c

OUTPUT1=./master
OUTPUT2=./slave

.PHONY: all clean

all:
	${CC} ${SOURCE1} ${CFLAGS} -o ${OUTPUT1}
	${CC} ${SOURCE2} ${CFLAGS} -o ${OUTPUT2}

clean:
	rm -f ${OUTPUT1} ${OUTPUT2}
