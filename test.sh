#!/bin/bash

# decode
./build/a.out -i test/DeskImage.jpg -o output.yuv
# encode
./build/a.out -i output.yuv -s 3200x2000 -o output.jpg

