#!/bin/bash

for file in tests/*.scm
do
    ./lisp_i $file
    printf "\n"
    echo "FINISHED {$file}"
done
