CC=gcc
CFLAGS=-Wall -pthread

SOURCE=./program.c

OUTPUT=./program

.PHONY: all clean test

all:
	${CC} ${SOURCE} ${CFLAGS} -o ${OUTPUT}

clean:
	rm -f ${OUTPUT}

test:
	-@rm Times.txt
	for filter in 8 20; do \
		for thread in 1 2 4 8; do \
			${OUTPUT} $${thread} coins.ascii.pgm filter$${filter}.txt out.pgm ; \
		done ; \
	done
