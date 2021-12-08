CFLAGS = -Wall -pedantic -Wstrict-prototypes -O3
LDLIBS = -lm
CC=cc

all: lisp printer

lisp: repl.c lisp.h
	${CC} repl.c -o $@ ${CFLAGS} ${LDLIBS}

printer: printer.c lisp.h
	${CC} printer.c -o $@ ${CFLAGS} ${LDLIBS}
