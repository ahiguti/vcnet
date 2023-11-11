#!/bin/bash

cd `dirname $0`

libs='usb_mod vcnet_usb vcnet_ir bitbang_sender bitbang_receiver vcnet_server'

for i in $libs; do
  rsync -av ~/user/Documents/Arduino/libraries/$i/ ./$i/
done
