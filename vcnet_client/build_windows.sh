#!/bin/bash

drive="/cygdrive/c"
is_wsl=`uname -r | grep microsoft`
if [ "$is_wsl" != "" ]; then
        drive="/mnt/c"
fi
mkdir -p ./x64/Release
mkdir -p ./x64/Debug
cp -f "$drive/build/SDL/VisualC/x64/Release/SDL2.dll" ./x64/Release/
cp -f "$drive/build/SDL/VisualC/x64/Debug/SDL2.dll" ./x64/Debug/
cp -f "$drive/build/SDL_ttf/VisualC/x64/Release/"*.dll ./x64/Release/
cp -f "$drive/build/SDL_ttf/VisualC/x64/Debug/"*.dll ./x64/Debug/
cp -f "$drive/build/glew/bin/Release/x64/"*.dll ./x64/Release/
cp -f "$drive/build/glew/bin/Debug/x64/"*.dll ./x64/Debug/
cp -f "$drive/build/ext/mplus-1m-bold.ttf" .
# rm -f ./x64/Release/vcnet_client.exe

exe="./x64/Release/vcnet_client.exe"
src="./vcnet.cpp"
if [ ! -f "$exe" -o "$src" -nt "$exe" ]; then
  time "$drive/Program Files (x86)/Microsoft Visual Studio/2019/Professional/Common7/IDE/devenv.exe" vcnet_client.sln /Build "Release|x64"
  e="$?"
  cat ./x64/Release/vcnet_client.log
  if [ "$e" != "0" ]; then
    exit 1
  fi
fi
exec ./x64/Release/vcnet_client.exe $*
