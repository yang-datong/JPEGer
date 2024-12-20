#!/bin/bash
set -e

wxh=512x512
file=tmp.yuv
#wxh=3200x2000
#file=De


if [ -f demo.jpg ]; then
	rm demo.jpg
fi
./build/a.out -i test/lenna.jpg -o $file
./build/a.out -i $file -s $wxh -o demo.jpg
if [ -f demo.jpg ]; then
	open demo.jpg
fi

