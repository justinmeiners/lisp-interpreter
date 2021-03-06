#!/bin/bash

cd tests/

for FILE in *.scm
do
    echo "$FILE"
    ../lisp --script "$FILE"
    RESULT=$?

    printf "\n"
    if [ $RESULT = "0" ]
    then
        echo "FINISHED $FILE"
    else
        echo "*FAILED* $FILE"
    fi
    printf "\n"
done

