CC=gcc
CFLAGS=-Wall

SOURCE=program.c
TEST_AS=testing_program_as_50mb.c
TEST_CPU=testing_program_cpu.c

OUTPUT=./program
OUTPUT_AS=./testing_program_as_50bmb
OUTPUT_CPU=./testing_program_cpu

BATCH_AS=example_batch_fail_as_50mb_10_10
BATCH_CPU=example_batch_fail_cpu_10_1000

.PHONY: all test clean

all:
	${CC} ${SOURCE} ${CFLAGS} -o ${OUTPUT}
	${CC} ${TEST_AS} ${CFLAGS} -o ${OUTPUT_AS}
	${CC} ${TEST_CPU} ${CFLAGS} -o ${OUTPUT_CPU}

test_as:
	${OUTPUT} ${BATCH_AS} 10 10

test_cpu:
	${OUTPUT} ${BATCH_CPU} 10 1000

clean:
	rm -f ${OUTPUT} ${OUTPUT_AS} ${OUTPUT_CPU}
