CFLAGS = -Wall -O3
LDLIBS = -lm
CC=cc

lisp: repl.c lisp.c lisp.h
	${CC} repl.c lisp.c -o $@ ${CFLAGS} ${LDLIBS}
