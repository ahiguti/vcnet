#!/bin/bash

cd `dirname $0`
rsync -av ~/user/Documents/Arduino/libraries/usb_mod/ ./usb_mod/
