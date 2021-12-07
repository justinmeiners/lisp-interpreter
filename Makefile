CFLAGS = -Wall -O3 -pedantic -Wstrict-prototypes
LDLIBS = -lm
CC=cc

all: lisp printer

lisp: repl.c lisp.h
	${CC} repl.c -o $@ ${CFLAGS} ${LDLIBS}

printer: printer.c lisp.h
	${CC} printer.c -o $@ ${CFLAGS} ${LDLIBS}
