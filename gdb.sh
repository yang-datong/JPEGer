#!/bin/bash

#设置动态库执行路径，在动态库存在于用户目录时
#export LD_LIBRARY_PATH=libavcodec:libavdevice:libavfilter:libavformat:libavresample:libavutil:libpostproc:libswresample:libswscale

#程序
program="build/a.out"

#程序执行需要的参数
args="-i output.yuv -s 512x512 -o demo.jpg"
args="-i demo.jpg -o output.yuv"

#源代码路径(Glibc路径,Option)
#ldd_version=$(ldd --version | grep ldd | awk '{print $5}')

#source_dir="libavcodec:libavdevice:libavfilter:libavformat:libavresample:libavutil:libpostproc:libswresample:libswscale"
#source_dir="/usr/src/glibc/glibc-${ldd_version}"

#最后传入脚步参数，配合这Vim-GDB使用
if [ "$(uname)" == "Linux" ]; then
  gdb ${program} -ex "set args ${args}" -ex "directory ${source_dir}" -ex "b main" -ex "r" "$@" -ex "c"
else
  lldb ${program} -o "run ${args}" -o "b main" -o "r" "$@" -o "c"
fi
