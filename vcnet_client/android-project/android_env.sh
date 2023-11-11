#!/bin/bash

ndk_ver="`/bin/ls $HOME/Android/Sdk/ndk/ | sort -n | head -1`"
ndk_dir="$HOME/Android/Sdk/ndk/$ndk_ver"

export ANDROID_HOME=~/Android/Sdk
export ANDROID_NDK_HOME="$ndk_dir"

PATH="$ANDROID_NDK_HOME:$PATH"
PATH="$ANDROID_HOME/platform-tools:$PATH"

