#!/bin/sh

PASS=1

cd code

for FILE in *.scm
do
    echo "$FILE"
    ../../lisp --script "$FILE"
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

cd ../
cd data

( ./test.sh )
RESULT=$?

if [ $RESULT = "0" ]
then
    echo "FINISHED data test"
else
    echo "*FAILED* data test"
    PASS=0
fi

if [ $PASS = "0" ]
then
  echo "**TESTS FAILED**"
else
  echo "**TESTS PASSED**"
fi
