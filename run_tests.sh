#!/bin/bash

cd tests/

PASS=1

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
        PASS=0
    fi
    printf "\n"
done

if [ $PASS = "0" ]
then
  echo "**TESTS FAILED**"
else
  echo "**TESTS PASSED**"
fi
