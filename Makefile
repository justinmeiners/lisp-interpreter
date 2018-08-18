TARGET = lisp_i
SRC = *.c
CFLAGS = -O3 -Wall
LDLIBS = -lm
CC = cc

${TARGET}: ${SRC}
	${CC} $^ -o $@ ${CFLAGS} ${LDLIBS}
