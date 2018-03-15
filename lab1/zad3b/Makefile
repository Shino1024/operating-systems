ifneq (${MAKECMDGOALS},clean)
ifndef OPT
$(error Please, set the OPT flag like make OPT=1 program_static)
endif
endif

CC=gcc
CFLAGS=-Wall

AR=ar
ARFLAGS=rcs

.PHONY: static shared dynamic clean

make all: program_shared program_static program_dynamic

static:
	${CC} -c condensed_lib.c ${CFLAGS} -O${OPT} -o condensed_lib.o
	${AR} ${ARFLAGS} libcondensed_lib.a condensed_lib.o

shared:
	${CC} ${CFLAGS} -fPIC -shared condensed_lib.c -O${OPT} -o libcondensed_lib.so

dynamic:
	${CC} ${CFLAGS} -fPIC -shared condensed_lib.c -O${OPT} -o libcondensed_lib.so

program_shared: shared
	${CC} program.c ${CFLAGS} -O${OPT} -DSHARED_VERSION -L. -Wl,-rpath=. -lcondensed_lib -o program_shared

program_static: static
	${CC} -c program.c ${CFLAGS} -O${OPT} -DSTATIC_VERSION
	${CC} program.o libcondensed_lib.a -O${OPT} -o program_static

program_dynamic: dynamic
	${CC} program.c -DDYNAMIC_VERSION -ldl ${CFLAGS} -O${OPT} -o program_dynamic

clean:
	rm *.o *.so *.a program_static program_dynamic program_shared
