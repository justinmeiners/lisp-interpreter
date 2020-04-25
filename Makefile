CFLAGS = -O3 -Wall
LDLIBS = -lm
CC=cc

lisp_i: lisp_i.c lisp.c lisp.h
	${CC} lisp_i.c lisp.c -o $@ ${CFLAGS} ${LDLIBS}
