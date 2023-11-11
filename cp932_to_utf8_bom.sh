#!/bin/bash

if [ ! -f "$1" ]; then
        exit 1
fi

echo -ne '\xef\xbb\xbf' > "$1.tmp" 
iconv -f cp932 -t utf8 "$1" >> "$1.tmp"
mv "$1.tmp" "$1"

