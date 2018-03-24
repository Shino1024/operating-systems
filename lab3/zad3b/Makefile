CC=gcc
CFLAGS=-Wall

SOURCE=program.c
TEST_AS=testing_program_as_50mb.c
TEST_CPU=testing_program_cpu.c

OUTPUT=./program
OUTPUT_AS=./testing_program_as_50bmb
OUTPUT_CPU=./testing_program_cpu

BATCH_AS=example_batch_fail_as_50mb_10_10
BATCH_CPU=example_batch_fail_cpu_1000_10

.PHONY: all test clean

all:
	${CC} ${SOURCE} ${CFLAGS} -o ${OUTPUT}
	${CC} ${TEST_AS} ${CFLAGS} -o ${OUTPUT_AS}
	${CC} ${TEST_CPU} ${CFLAGS} -o ${OUTPUT_CPU}

test_as:
	${OUTPUT} 10 10 ${BATCH_AS}

test_cpu:
	${OUTPUT} 1000 10 ${BATCH_CPU}

clean:
	rm -f ${OUTPUT} ${OUTPUT_AS} ${OUTPUT_CPU}
