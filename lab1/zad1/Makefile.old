CC=gcc
AR=ar
ARFLAGS=rcs
STATIC_LIB=libblock_array.a
SHARED_LIB=libblock_array.so
IMP_DIR=src/imp
SRC=$(shell find "${IMP_DIR}/" -type f -name "*.c" -exec basename {} ';')
OBJ_DIR=obj
OBJ_FPIC_DIR=objfpic
OBJS=$(addprefix ${OBJ_DIR}/,${SRC:.c=.o})
OBJS_FPIC=$(addprefix ${OBJ_FPIC_DIR}/,${SRC:.c=.o})
LIB_DIR=lib

CFLAGS=-Wall
EXTRA_CFLAGS_0=-fPIC
EXTRA_CFLAGS_1=-shared

${OBJ_DIR}/%.o: ${IMP_DIR}/%.c
	${CC} -c $< ${CFLAGS} -o ${OBJ_DIR}/$@

${OBJS}: ${OBJ_DIR}/%.o: ${IMP_DIR}/%.c
	${CC} -c $< ${CFLAGS} -o $@

static: ${OBJS}
	${AR} ${ARFLAGS} ${LIB_DIR}/${STATIC_LIB} $^

${OBJ_FPIC_DIR}/%.o: ${IMP_DIR}/%.c
	${CC} -c $< ${EXTRA_CFLAGS_0} ${CFLAGS} -o ${OBJ_FPIC_DIR}/$@

${OBJS_FPIC}: ${OBJ_FPIC_DIR}/%.o: ${IMP_DIR}/%.c
	${CC} -c $< ${EXTRA_CFLAGS_0} ${CFLAGS} -o $@

shared: ${OBJS_FPIC}
	${CC} -shared -Wl,-soname,${SHARED_LIB}.1.0.0 -o ${LIB_DIR}/${SHARED_LIB}.1.0.0 $^ -lc

.PHONY: all clean

all: static shared

clean:
	@rm lib/* obj/* objfpic/*
