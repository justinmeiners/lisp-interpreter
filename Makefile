CFLAGS = -Wall -pedantic -O3 -Wstrict-prototypes
LDLIBS = -lm
CC=cc

lisp: repl.c lisp.h
	${CC} repl.c -o $@ ${CFLAGS} ${LDLIBS}
