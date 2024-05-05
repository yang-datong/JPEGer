#include "BMP.hpp"
#include "Image.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <ios>

File::BMP::BMP() {}
File::BMP::~BMP() { close(); }

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
  RGBToYUV();
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

int File::BMP::RGBToYUV() {
  _YUVSize = _height * _width * 3;
  int size = _height * _width;
  _Y = new uint8_t[size];
  _U = new uint8_t[size];
  _V = new uint8_t[size];
  int offset = -1;
  for (int i = 0; i < _height; i++) {
    for (int j = 0; j < _width; j++) {
      offset = i * _height + j;
      uint8_t r = _rgbBuffer[offset * 3];
      uint8_t g = _rgbBuffer[offset * 3 + 1];
      uint8_t b = _rgbBuffer[offset * 3 + 2];

      _Y[offset] = round(0.299f * r + 0.587f * g + 0.114f * b);
      _U[offset] = round(-0.1687f * r - 0.3313f * g + 0.5f * b + 128);
      _V[offset] = round(0.5f * r - 0.4187f * g - 0.0813f * b + 128);
    }
  }
  _yuv444PlanarBuffer = new uint8_t[_YUVSize];
  memcpy(_yuv444PlanarBuffer, _Y, size);
  memcpy(_yuv444PlanarBuffer + size, _U, size);
  memcpy(_yuv444PlanarBuffer + size * 2, _V, size);

  _yuv444PackedBuffer = new uint8_t[_YUVSize];
  Image::YUV444PlanarToPacked(_yuv444PlanarBuffer, _yuv444PackedBuffer, _width,
                              _height);
  delete[] _Y;
  _Y = nullptr;
  delete[] _U;
  _U = nullptr;
  delete[] _V;
  _V = nullptr;
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
  if (_Y) {
    delete[] _Y;
    _Y = nullptr;
  }
  if (_U) {
    delete[] _U;
    _U = nullptr;
  }
  if (_V) {
    delete[] _V;
    _V = nullptr;
  }
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
