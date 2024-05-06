# Version 3
一个最简单的JPEG-JFIF解码器，编、解码功能单一，按照JPEG-JFIF标准写的编、解码器，很多数值都是写死的（不具有动态修改）。

**已有功能**：可以编、解码标准的JPEG-JFIF文件，DCT/IDCT多线程

## 问题

1. 遇到非8整数的宽高图片会有问题
2. 编、解码速度过慢
3. 未经过批量测试，大部分图片都无法正常编、接码

## SIMD加速（目前只有解码，且输出图像有问题TODO）
本人电脑不支持AVX，故使用SSE，测试用例如下：

1. 解码大小为1.4M的JPG图片，分辨率为3200x2000（非SIMD）：
```bash
$ time ./a.out -i DeskImage.jpg -o demo.yuv
________________________________________________________
Executed in   58.23 secs    fish           external
   usr time   57.56 secs  269.00 micros   57.55 secs
   sys time    0.66 secs   32.00 micros    0.66 secs
```

2. 解码大小为1.4M的JPG图片，分辨率为3200x2000（SSE）：
```bash
$ time ./a.out -i DeskImage.jpg -o demo.yuv
________________________________________________________
Executed in   37.62 secs    fish           external
   usr time   36.99 secs  425.00 micros   36.99 secs
   sys time    0.63 secs   56.00 micros    0.63 secs
```
>在main.cpp中添加#define SSE 然后重新编译，即可执行SSE优化函数.

## 多线程

### 解码

1. 解码大小为1.4M的JPG图片，分辨率为3200x2000（单线程）：

```bash
$ time ./a.out -i DeskImage.jpg -o DeskImage.yuv 
________________________________________________________
Executed in   55.49 secs    fish           external
   usr time   55.00 secs  271.00 micros   55.00 secs
   sys time    0.48 secs   18.00 micros    0.48 secs
$ md5sum DeskImage.yuv        
6a8baba370d2befa329d89a0910e7b1f  DeskImage.yuv
```

2. 解码大小为1.4M的JPG图片，分辨率为3200x2000（多线程）：

```bash
$ time ./a.out -i DeskImage.jpg -o DeskImage.yuv
________________________________________________________
Executed in   28.40 secs    fish           external
   usr time   11.46 secs  461.00 micros   11.46 secs
   sys time    1.00 secs   75.00 micros    1.00 secs
$ md5sum DeskImage.yuv 
6a8baba370d2befa329d89a0910e7b1f  DeskImage.yuv
```

- 其中IDCT使用3个多线程分别计算Y,U,V分量

### 编码

1. 编码大小为19M的YUV444p图片，分辨率为3200x2000（单线程）：

```bash
$ time ./a.out -i DeskImage.yuv -s 3200x2000 -o demo.jpg 
________________________________________________________
Executed in   66.58 secs    fish           external
   usr time   66.36 secs  275.00 micros   66.36 secs
   sys time    0.22 secs    9.00 micros    0.22 secs
$ md5sum demo.jpg    
4ffd014f38bbaee96d2ef0e6cf4fcb3e  demo.jpg
```

2. 编码大小为19M的YUV444p图片，分辨率为3200x2000（多线程）：

```bash
$ time ./a.out -i DeskImage.yuv -s 3200x2000 -o demo.jpg 
________________________________________________________
Executed in   39.70 secs    fish           external
   usr time   22.62 secs    0.00 micros   22.62 secs
   sys time    1.35 secs  217.00 micros    1.35 secs
$ md5sum demo.jpg    
4ffd014f38bbaee96d2ef0e6cf4fcb3e  demo.jpg
```

- 其中DCT使用3个多线程分别计算Y,U,V分量

### 后续

- [x] 模块化项目
- [x] 添加开发“编码器”
- [ ] 添加simd加速解码
- [ ] 添加循环解码支持
- [ ] 添加多线程支持（至少4个线程）
