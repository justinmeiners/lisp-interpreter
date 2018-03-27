#!/bin/bash

cd tests/

for file in *.scm
do
    echo "../lisp_i --load ${file}"
    ../lisp_i --load $file
    echo "FINISHED"
    printf "\n"
done

