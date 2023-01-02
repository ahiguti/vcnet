#!/bin/bash

cd `dirname $0`

#rm -rf *.cache *.hw *.impl *.ip_user_files *.runs *.sim *.jou *.log .Xil
#rm -rf *.srcs/*/bd/*/ip
#rm -rf *.srcs/*/bd/*/ipshared
#rm -rf *.mcs *.prm

rm -rf .metadata  .analytics */Debug */Release RemoteSystemsTempFiles
find . -name "*.o" -exec rm {} \; 2> /dev/null
find . -name "*.elf" -exec rm {} \; 2> /dev/null
find . -name "*.log" -exec rm -rf {} \; 2> /dev/null

# rm -f BOOT.bin output.bif

rm -rf */_ide
rm -rf */export
rm -rf */logs
rm -rf */hw
rm -rf */platform.tcl
rm -rf */tempdsa

#以下を消すとxpfm生成に失敗する
#rm -rf */ps7_cortexa9_0
#rm -rf */zynq_fsbl
