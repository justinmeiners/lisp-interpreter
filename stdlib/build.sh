#!/bin/bash

SRC=*.scm

cat lisp_lib.h

echo "#ifdef LISP_IMPLEMENTATION"

for FILE in $SRC
do
    echo "static const char* lib_$(basename $FILE .scm)_src_ = "
    cat $FILE | ./text2c.sh 
    echo ";"
    echo ""
done

cat lisp_lib.c

echo "#endif"
