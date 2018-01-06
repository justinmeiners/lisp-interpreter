TARGET = lisp_i
SRC = *.c
FLAGS = -O3
CC = gcc

${TARGET}: ${SRC}
	${CC} ${FLAGS} $^ -o $@
