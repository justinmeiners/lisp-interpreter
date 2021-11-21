CFLAGS = -Wall -pedantic -Wstrict-prototypes -O3
LDLIBS = -lm
CC=cc

lisp: repl.c lisp.h
	${CC} repl.c -o $@ ${CFLAGS} ${LDLIBS}
