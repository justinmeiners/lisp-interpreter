CFLAGS = -Idist/ -Wall -pedantic -Wstrict-prototypes -O3
LDLIBS = -lm
CC=cc

all: lisp printer sample

clean:
	rm -f lisp
	rm -f printer
	rm -f sample
	rm -f dist/lisp_lib.h

lisp: repl.c dist/lisp.h dist/lisp_lib.h
	${CC} repl.c -o $@ ${CFLAGS} ${LDLIBS}

printer: printer.c dist/lisp.h
	${CC} printer.c -o $@ ${CFLAGS} ${LDLIBS}

sample: sample.c dist/lisp.h dist/lisp_lib.h
	${CC} sample.c -o $@ ${CFLAGS} ${LDLIBS}

dist/lisp_lib.h: stdlib/lib.h stdlib/lib.c
	cd stdlib; ./concat.sh > ../$@;


.PHONY: all clean
