#!/bin/bash

cd tests/

for file in *.scm
do
    ../lisp_i --load $file
    printf "\n"
    echo "FINISHED {$file}"
done

