CFLAGS = -Wall -O3 -pedantic -Wstrict-prototypes
LDLIBS = -lm
CC=cc

lisp: repl.c lisp.h
	${CC} repl.c -o $@ ${CFLAGS} ${LDLIBS}
