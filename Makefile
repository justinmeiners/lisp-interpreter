TARGET = lisp_i
SRC = *.c
FLAGS = -O3 -Wall
CC = gcc

${TARGET}: ${SRC}
	${CC} ${FLAGS} $^ -o $@
