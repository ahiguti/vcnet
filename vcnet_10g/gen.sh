#!/bin/bash

if [ "$1" = "" ]; then
  exit 1
fi

egrep '^[0-9A-F]+ [0-9A-F]+ [0-9A-F]+ ;' $1 | \
awk -e '{ printf("{ 0x%s, 0x%s, 0x%s }, // %s\n", $1, $2, $3, $0) }' > $1.c

egrep '^[0-9A-F]+ [0-9A-F]+ [0-9A-F]+' $1 | \
awk -e 'BEGIN{i=0}{printf("%03d: init_reg = 24'"'"'h%s_%s_%s; // %s\n", i++, $1, $2, $3, $0)}' | sed -e 's///g' > $1.v
