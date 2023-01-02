#!/bin/bash

cd `dirname $0`

# mkdir -p ./build
# find ./*.runs \( -name "*.ltx" \) -exec cp -f {} ./build/ \; 2> /dev/null

rm -rf *.cache *.hw *.impl *.ip_user_files *.runs *.sim *.jou *.log .Xil
rm -rf *.mcs *.prm
rm -rf *.srcs/*/bd/*/ip
rm -rf *.srcs/*/bd/*/ipshared
rm -rf *.srcs/*/bd/*/ui
rm -rf *.srcs/*/bd/*/hw_handoff
rm -rf *.srcs/*/bd/*/synth
rm -rf *.srcs/*/bd/*/sim
rm -rf *.srcs/*/bd/mref
