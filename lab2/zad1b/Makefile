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
		for RECORD_NUM in 2000 4000 ; do \
			${OUTPUT} generate ${PREFIX}_$${RECORD_NUM}_$${RECORD_LEN} $${RECORD_NUM} $${RECORD_LEN} ; \
			for MODE in lib sys ; do \
				cp ${PREFIX}_$${RECORD_NUM}_$${RECORD_LEN} ${PREFIX}_$${RECORD_NUM}_$${RECORD_LEN}_$${MODE} ; \
				${OUTPUT} sort ${PREFIX}_$${RECORD_NUM}_$${RECORD_LEN}_$${MODE} $${RECORD_NUM} $${RECORD_LEN} $${MODE} ; \
				${OUTPUT} copy ${PREFIX}_$${RECORD_NUM}_$${RECORD_LEN}_$${MODE} ${PREFIX}_$${RECORD_NUM}_$${RECORD_LEN}_$${MODE}_copy $${RECORD_NUM} $${RECORD_LEN} $${MODE} ; \
			done ; \
		done ; \
	done

clean:
	rm -f ${OUTPUT} ${PREFIX}*
