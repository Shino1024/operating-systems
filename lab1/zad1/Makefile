CC=gcc
CFLAGS=-Wall

AR=ar
ARFLAGS=rcs

.PHONY: static shared clean

make all: static shared test_shared

static:
	${CC} -c condensed_lib.c ${CFLAGS} -o condensed_lib.o
	${AR} ${ARFLAGS} condensed_lib.a condensed_lib.o

shared:
	${CC} -fPIC -shared condensed_lib.c -o libcondensed_lib.so

test_shared: shared
	${CC} test.c -L. -Wl,-rpath=. -lcondensed_lib -o test

clean:
	rm *.o *.so *.a