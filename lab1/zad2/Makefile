CC=gcc
CFLAGS=-Wall

AR=ar
ARFLAGS=rcs

.PHONY: static shared clean

make all: static shared program_shared program_static

static:
	${CC} -c condensed_lib.c ${CFLAGS} -o condensed_lib.o
	${AR} ${ARFLAGS} libcondensed_lib.a condensed_lib.o

shared:
	${CC} -fPIC -shared condensed_lib.c -o libcondensed_lib.so

program_shared: shared
	${CC} program.c -L. -Wl,-rpath=. -lcondensed_lib -o program_shared

program_static: static
	${CC} -c program.c
	${CC} program.o libcondensed_lib.a -o program_static

clean:
	rm *.o *.so *.a