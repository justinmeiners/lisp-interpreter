CFLAGS = -Wall -pedantic -Wstrict-prototypes -O3
LDLIBS = -lm
CC=cc

all: lisp printer sample

lisp: repl.c lisp.h
	${CC} repl.c -o $@ ${CFLAGS} ${LDLIBS}

printer: printer.c lisp.h
	${CC} printer.c -o $@ ${CFLAGS} ${LDLIBS}

sample: sample.c lisp.h
	${CC} sample.c -o $@ ${CFLAGS} ${LDLIBS}
