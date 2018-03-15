CC=gcc
CFLAGS=-Wall

AR=ar
ARFLAGS=rcs

.PHONY: static shared clean

make all: static shared

static:
	${CC} -c condensed_lib.c ${CFLAGS} -o condensed_lib.o
	${AR} ${ARFLAGS} libcondensed_lib.a condensed_lib.o

shared:
	${CC} -fPIC -shared condensed_lib.c -o libcondensed_lib.so

clean:
	rm *.o *.so *.a
