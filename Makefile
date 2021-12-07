CFLAGS = -Wall -pedantic -Wstrict-prototypes -g
LDLIBS = -lm
CC=cc

lisp: repl.c lisp.h
	${CC} repl.c -o $@ ${CFLAGS} ${LDLIBS}
