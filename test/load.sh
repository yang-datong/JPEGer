#!/bin/bash

set -e
if [ -f md5.log ];then
	rm md5.log
fi

decode(){
	local type=$1
	for((i=1;i<6;i++));do
		./a.out -i lenna.jpg -o lenna_$i.$type
		echo "./a.out -i lenna.jpg -o lenna_$i.$type " >> md5.log
		md5sum lenna_$i.$type >> md5.log
		rm lenna_$i.$type
	done
}

encode(){
	local type=$1
	for((i=1;i<6;i++));do
		if [ "$type" == "yuv" ];then
			./a.out -i lenna.$type -o demo_$i.jpg -s 512x512
		else
			./a.out -i lenna.$type -o demo_$i.jpg
		fi
		echo "./a.out -i lenna.$type -o demo_$i.jpg " >> md5.log
		md5sum demo_$i.jpg >> md5.log
		rm demo_$i.jpg
	done
}


if [ ! -f lenna.jpg ] || [ ! -f lenna.yuv ] || [ ! -f lenna.bmp ];then
	echo -e "\033[31m需要测试文件 lenna.jpg lenna.yuv lenna.bmp\033[0m";exit
fi

decode yuv
decode ppm

encode yuv
encode bmp


