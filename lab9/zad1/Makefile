CC=gcc
CFLAGS=-Wall -pthread

SOURCE1=./programA.c
SOURCE2=./programB.c

OUTPUT1=./programA
OUTPUT2=./programB

.PHONY: all clean

all:
	${CC} ${SOURCE1} ${CFLAGS} -o ${OUTPUT1}
	${CC} ${SOURCE2} ${CFLAGS} -o ${OUTPUT2}

clean:
	rm -f ${OUTPUT1} ${OUTPUT2}
