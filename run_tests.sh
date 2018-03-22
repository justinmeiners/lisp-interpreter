#!/bin/bash

for file in test_code/*.scm
do
    ./lisp_i --file $file
    printf "\n"
    echo "FINISHED {$file}"
done

for file in test_data/*.scm
do
    ./lisp_i --quote --file $file
    printf "\n"
    echo "FINISHED {$file}"
done
