#!/bin/sh

# make sure reader and writer work and are compatible.

# 1. load s expresion, print it out.
cat sample.scm | ../../printer > temp1.scm

# 2. load the output, print it out again.
cat temp1.scm | ../../printer > temp2.scm

# 3. ensure both outputs match
cmp temp1.scm temp2.scm


