CC=gcc
CFLAGS=-Wall

SOURCE=./program.c
OUTPUT=./program

PREFIX=test_data

.PHONY: all clean test

all:
	${CC} ${SOURCE} ${CFLAGS} -o ${OUTPUT}

test:
	for RECORD_LEN in 4 512 4096 8192 ; do \
		for RECORD_NUM in 500 1000 ; do \
			${OUTPUT} generate ${PREFIX}_$${RECORD_NUM}_$${RECORD_LEN} $${RECORD_NUM} $${RECORD_LEN} ; \
			for MODE in lib sys ; do \
				${OUTPUT} sort ${PREFIX}_$${RECORD_NUM}_$${RECORD_LEN} $${RECORD_NUM} $${RECORD_LEN} $${MODE} ; \
				${OUTPUT} copy ${PREFIX}_$${RECORD_NUM}_$${RECORD_LEN} ${PREFIX}_$${RECORD_NUM}_$${RECORD_LEN}_copy $${RECORD_NUM} $${RECORD_LEN} $${MODE} ; \
			done ; \
		done ; \
	done

clean:
	rm -f ${OUTPUT} ${PREFIX}*
