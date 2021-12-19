CFLAGS = -Wall -pedantic -Wstrict-prototypes -O3
LDLIBS = -lm
CC=cc

all: lisp printer sample

clean:
	rm -f lisp
	rm -f printer
	rm -f sample
	rm -f lisp_lib.h

lisp: repl.c lisp.h lisp_lib.h
	${CC} repl.c -o $@ ${CFLAGS} ${LDLIBS}

printer: printer.c lisp.h
	${CC} printer.c -o $@ ${CFLAGS} ${LDLIBS}

sample: sample.c lisp.h lisp_lib.h
	${CC} sample.c -o $@ ${CFLAGS} ${LDLIBS}

lisp_lib.h: lib/lisp_lib.h lib/lisp_lib.c
	cd lib; ./build.sh > ../$@;


.PHONY: all clean
