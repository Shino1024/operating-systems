CC=gcc
CFLAGS=-Wall

SOURCE1=./program1.c
SOURCE2=./program2.c

OUTPUT1=./program1
OUTPUT2=./program2

.PHONY: all clean

all:
	${CC} ${SOURCE1} ${CFLAGS} -o ${OUTPUT1}
	${CC} ${SOURCE2} ${CFLAGS} -o ${OUTPUT2}

clean:
	rm -f ${OUTPUT1} ${OUTPUT2}
