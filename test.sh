#!/bin/bash

set -e

#TODO: DeskImage解码没问题，编码有问题
deskimage(){
  # decode
  ./build/a.out -i test/DeskImage.jpg -o output.yuv
  # encode
  ./build/a.out -i output.yuv -s 3200x2000 -o output.jpg
}

lenna(){
  # decode
  ./build/a.out -i test/lenna.jpg -o output.yuv
  # encode
  ./build/a.out -i output.yuv -s 512x512 -o output.jpg
}

test1(){
  # decode
  ./build/a.out -i test/1080p_yuvj444p_base.jpg -o output.yuv
  # encode
  ./build/a.out -i output.yuv -s 1920x1080 -o output.jpg
}

#deskimage
lenna
#test1
