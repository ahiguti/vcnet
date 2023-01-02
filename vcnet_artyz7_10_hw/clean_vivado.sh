#!/bin/bash

cd `dirname $0`

rm -rf *.cache *.hw *.impl *.ip_user_files *.runs *.sim *.jou *.log .Xil
rm -rf *.mcs *.prm

rm -rf */*/bd/*/ip
rm -rf */*/bd/*/ipshared
rm -rf */*/bd/*/ui
rm -rf */*/bd/*/hw_handoff
rm -rf */*/bd/*/synth
rm -rf */*/bd/*/sim
rm -rf */*/bd/mref

rm -rf */*/bd/*/*.bda
rm -rf */*/bd/*/*.bxml
rm -rf */*/bd/*/*.xdc

# rm -rf *.xsa

