# Version 3
JPEG-JFIF解码器，具有编、解码功能，按照JPEG-JFIF标准编写且符合JFIF协议，很多数值都是写死的（不具有动态修改）。

**已有功能**：能编、解码标准的JPEG-JFIF文件，具体有DCT/IDCT多线程、SIMD(SSE,AVX)加速功能。

## 问题

1. 遇到非8整数的宽高图片会有问题
2. 原生编、解码速度过慢（不考虑加速操作）
3. 未经过批量测试，部分图片可能无法正常编、接码



## 解码加速

解码大小为1.4M的JPG图片，分辨率为3200x2000（无任何额外加速）：

```bash
$ time ./a.out -i DeskImage.jpg -o demo.yuv
________________________________________________________
Executed in   43.83 secs    fish           external
   usr time   43.47 secs    0.00 micros   43.47 secs
   sys time    0.35 secs  429.00 micros    0.35 secs
   
$ md5sum demo.yuv 
6a8baba370d2befa329d89a0910e7b1f  output.yuv
```

### SIMD加速

1. 解码大小为1.4M的JPG图片，分辨率为3200x2000（SSE）：

```bash
$ time ./a.out -i DeskImage.jpg -o demo.yuv
________________________________________________________
Executed in   14.47 secs    fish           external
   usr time   14.11 secs  350.00 micros   14.11 secs
   sys time    0.36 secs   56.00 micros    0.36 secs

$ md5sum output.yuv                                                         
224b56dc489e18d6e58f8ec1f2513a28  output.yuv
```
>在MCU.cpp中添加#define SSE 然后重新编译，即可执行SSE优化函数.

2. 解码大小为1.4M的JPG图片，分辨率为3200x2000（AVX）：

```bash
$ time ./a.out -i DeskImage.jpg -o demo.yuv
________________________________________________________
Executed in   13.15 secs    fish           external
   usr time   12.78 secs    0.00 micros   12.78 secs
   sys time    0.35 secs  442.00 micros    0.35 secs

$ md5sum output.yuv
6817b350317285861969dcb57d2fc46a  output.yuv
```

>在MCU.cpp中添加#define AVX 然后重新编译，即可执行AVX优化函数.

### 多线程

1. 解码大小为1.4M的JPG图片，分辨率为3200x2000（多线程，3线程Y、U、V分别一个线程）：

```bash
$ time ./a.out -i DeskImage.jpg -o demo.yuv
________________________________________________________
Executed in   23.48 secs    fish           external
   usr time   11.08 secs  302.00 micros   11.08 secs
   sys time    1.41 secs   58.00 micros    1.41 secs

$ md5sum output.yuv 
6a8baba370d2befa329d89a0910e7b1f  output.yuv
```
>在MCU.cpp中添加#define Threads 然后重新编译，即可执行3线程优化函数.


### 多线程+AVX加速

```bash
$ time ./a.out -i DeskImage.jpg -o demo.yuv
________________________________________________________
Executed in   11.71 secs    fish           external
   usr time    6.28 secs  357.00 micros    6.28 secs
   sys time    2.40 secs   77.00 micros    2.40 secs

$ md5sum output.yuv
6817b350317285861969dcb57d2fc46a  output.yuv
```

> 在MCU.cpp中添加#define Threads_AVX 然后重新编译，即可执行3线程优化函数.



## 编码加速

1. 编码大小为19M的YUV444p图片，分辨率为3200x2000（无任何额外加速）：

```bash
$ time ./a.out -i DeskImage.yuv -s 3200x2000 -o demo.jpg 
________________________________________________________
Executed in   49.93 secs    fish           external
   usr time   49.59 secs  312.00 micros   49.59 secs
   sys time    0.34 secs   64.00 micros    0.33 secs

$ md5sum demo.jpg    
4ffd014f38bbaee96d2ef0e6cf4fcb3e  demo.jpg
```

### SIMD加速

1. 编码大小为19M的YUV444p图片，分辨率为3200x2000（SSE）：

```bash
$ time ./a.out -i DeskImage.yuv -s 3200x2000 -o demo.jpg 
________________________________________________________
Executed in   21.78 secs    fish           external
   usr time   21.43 secs  537.00 micros   21.43 secs
   sys time    0.34 secs  109.00 micros    0.34 secs

$ md5sum demo.jpg
57f22903b2f28531d1a689bbd429aebf  demo.jpg
```

2. 编码大小为19M的YUV444p图片，分辨率为3200x2000（AVX）：

```bash
$ time ./a.out -i DeskImage.yuv -s 3200x2000 -o demo.jpg 
________________________________________________________
Executed in   19.85 secs    fish           external
   usr time   19.50 secs  413.00 micros   19.50 secs
   sys time    0.35 secs   85.00 micros    0.35 secs
$ md5sum demo.jpg 
fc2973627fad10510cc5b862219f2155  demo.jpg
```


### 多线程
1. 编码大小为19M的YUV444p图片，分辨率为3200x2000（多线程，3线程Y、U、V分别一个线程）：

```bash
$ time ./a.out -i DeskImage.yuv -s 3200x2000 -o demo.jpg 
________________________________________________________
Executed in   29.86 secs    fish           external
   usr time   17.34 secs    0.00 micros   17.34 secs
   sys time    1.83 secs  450.00 micros    1.83 secs

$ md5sum demo.jpg
4ffd014f38bbaee96d2ef0e6cf4fcb3e  demo.jpg
```



## 后续

- [x] 模块化项目
- [x] 添加开发“编码器”
- [x] 添加simd加速解码
- [ ] 添加循环解码支持（不打算支持了）
- [x] 添加多线程支持
