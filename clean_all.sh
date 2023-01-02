#!/bin/bash

cd `dirname $0`
make -C vcnet_client clean

cmd=`find . -name "clean_vivado.sh" -o -name "clean_vitis.sh"`

for c in $cmd; do
  echo "$c"
  "$c"
done

find . -name "*.log" -exec rm -rf {} \; 2> /dev/null
find . -name "*.bin" -exec rm -rf {} \; 2> /dev/null
find . -name "*.a" -exec rm -rf {} \; 2> /dev/null
find . -name ".analytics" -exec rm -rf {} \; 2> /dev/null
rm -rf Packages
