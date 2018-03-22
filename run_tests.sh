#!/bin/bash

for file in test_code/*.scm
do
    ./lisp_i $file
    printf "\n"
    echo "FINISHED {$file}"
done
