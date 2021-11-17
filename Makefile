CFLAGS = -Wall -O3
LDLIBS = -lm
CC=cc

lisp: repl.c lisp.h
	${CC} repl.c -o $@ ${CFLAGS} ${LDLIBS}
