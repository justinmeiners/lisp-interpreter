TARGET = lisp_i
SRC = *.c
FLAGS = 
CC = gcc

${TARGET}: ${SRC}
	${CC} ${FLAGS} $^ -o $@
