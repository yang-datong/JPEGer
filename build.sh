#!/bin/bash

if [ -d build ];then
  rm -rf build
fi
cmake -B build .
make -C build -j $(nproc)
