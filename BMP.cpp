#include "BMP.hpp"
#include "Image.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <ios>

File::BMP::BMP() {}
File::BMP::~BMP() { close(); }

/* TODO YangJing 有个问题，就是转成YUV后图片左右翻转了 <24-05-06 21:17:24> */
int File::BMP::BMPToRBG(string &bmpFilePath, string rgbFilePath) {
  ifstream bmp_file(bmpFilePath, ios::binary);
  if (!bmp_file.is_open()) {
    std::cerr << "Error opening BMP file!" << std::endl;
    return -1;
  }
  // 读取文件头
  bmp_file.read((char *)&head, sizeof(head));
  // 读取信息头
  bmp_file.read((char *)&info, sizeof(info));

  _width = info.width;
  _height = info.height;

  // 为像素数据分配内存
  int bufferSize = _width * _height * 3;
  uint8_t *buffer = new uint8_t[bufferSize];
  uint8_t *re_buffer = new uint8_t[bufferSize];

  // 读取RGB数据
  bmp_file.seekg(head.offset, std::ios_base::beg);
  bmp_file.read((char *)buffer, bufferSize);

  for (int i = 0; i < bufferSize; i++) {
    re_buffer[i] = buffer[(bufferSize - 1 - i)];
  }
  delete[] buffer;
  buffer = nullptr;

  _rgbBuffer = re_buffer;
  _rgbBufferSize = bufferSize;
  bmp_file.close();

  /* 不写文件则保留内存，等手动释放 */
  if (rgbFilePath.empty())
    return 0;

  ofstream outputFile(rgbFilePath, ios_base::binary);
  if (!outputFile.is_open()) {
    std::cerr << "Error opening RGG file!" << std::endl;
    return -1;
  }
  outputFile.write((const char *)_rgbBuffer, _rgbBufferSize);
  outputFile.close();
  std::cout << "Output RGB file: " + rgbFilePath << std::endl;

  /* 写文件则释放内存 */
  close();
  return 0;
}

int File::BMP::BMPToYUV(string &bmpFilePath, string yuvFilePath) {
  BMPToRBG(bmpFilePath);
  Image::RGBToYUV(_rgbBuffer, _width, _height, _YUVSize, _yuv444PlanarBuffer,
                  _yuv444PackedBuffer);
  if (yuvFilePath.empty())
    return 0;

  ofstream outputFile(yuvFilePath, ios_base::binary);
  if (!outputFile.is_open()) {
    std::cerr << "Error opening RGG file!" << std::endl;
    return -1;
  }
  outputFile.write((const char *)_yuv444PlanarBuffer, _YUVSize);
  outputFile.close();
  std::cout << "Output RGB file: " + yuvFilePath << std::endl;

  /* 写文件则释放内存 */
  close();
  return 0;
}

int File::BMP::RGBToBMP(string &rgbFilePath, int w, int h,
                        string &bmpFilePath) {
  printf("head byte -> %ld\n", sizeof(Head));
  printf("info byte -> %ld\n", sizeof(Info));
  FILE *fp_rgb24 = fopen(rgbFilePath.c_str(), "rb");
  FILE *fp_bmp = fopen(bmpFilePath.c_str(), "wb+");

  if (fp_rgb24 == NULL || fp_bmp == NULL) {
    printf("NULL!");
    return (-1);
  }

  unsigned char *rgb24_buf = (unsigned char *)malloc(w * h * 3);
  unsigned char *re_rgb24_buf = (unsigned char *)malloc(w * h * 3);
  fread(re_rgb24_buf, 1, w * h * 3, fp_rgb24);

  //倒置字节
  for (int i = 0; i < w * h * 3; ++i) {
    rgb24_buf[i] = re_rgb24_buf[((w * h * 3) - 1) - i];
  }

  int head_size = sizeof(Head) + sizeof(Info);
  printf("%d\n", head_size);
  head.file_size = head_size + w * h * 3;
  head.offset = head_size;

  info.info_size = sizeof(info);
  info.width = w;
  info.height = h;
  info.num_color_planes = 1;
  info.bit = 3 * 8; /* rgb24 */
  info.compress_type = 0;
  info.kernel_data_size = w * h * 3;
  info.horizontal_resolution = 0;
  info.vertical_resolution = 0;
  info.tab_index = 0;
  info.important_idx = 0;

  fwrite(&head, 1, sizeof(head), fp_bmp);
  fwrite(&info, 1, sizeof(info), fp_bmp);
  fwrite(rgb24_buf, 1, w * h * 3, fp_bmp);

  free(re_rgb24_buf);
  re_rgb24_buf = NULL; // uaf
  free(rgb24_buf);
  rgb24_buf = NULL;
  fclose(fp_rgb24);
  fclose(fp_bmp);
  return 0;
}

int File::BMP::close() {
  if (_rgbBuffer) {
    delete[] _rgbBuffer;
    _rgbBuffer = nullptr;
  }
  _rgbBufferSize = 0;
  if (_yuv444PlanarBuffer) {
    delete[] _yuv444PlanarBuffer;
    _yuv444PlanarBuffer = nullptr;
  }
  if (_yuv444PackedBuffer) {
    delete[] _yuv444PackedBuffer;
    _yuv444PackedBuffer = nullptr;
  }
  _YUVSize = 0;
  return 0;
}
