#!/bin/sh

cat big_data_gen.sexpr |  ../../lisp --script big_data1.scm
cat big_data_canada.sexpr |  ../../lisp --script big_data2.scm
