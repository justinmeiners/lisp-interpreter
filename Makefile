TARGET = lisp_i
SRC = *.c
FLAGS = -Wall -O3
CC = gcc

${TARGET}: ${SRC}
	${CC} ${FLAGS} $^ -o $@
