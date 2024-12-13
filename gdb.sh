#!/bin/bash

#程序执行需要的参数
args="-i output.yuv -s 512x512 -o demo.jpg"

#最后传入脚步参数，配合这Vim-GDB使用
gdb ./build/a.out -ex "set args ${args}" -ex "directory ${source_dir}" -ex "b main" -ex "r" "$@"
