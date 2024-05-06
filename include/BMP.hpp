#ifndef BMP_HPP_4ZTJJIQB
#define BMP_HPP_4ZTJJIQB

#include "Type.hpp"
#include <cstdint>

namespace File {
#pragma pack(push, 1)
typedef struct _Head {
  uint16_t magic{0x4D42};
  int32_t file_size{0};
  int32_t extension{0};
  int32_t offset{0};
} Head;

typedef struct _Info {
  int32_t info_size{0};
  int32_t width{0}; /* unit pixel */
  int32_t height{0};
  unsigned short num_color_planes{0};
  unsigned short bit{0};
  int32_t compress_type{0};
  int32_t kernel_data_size{0};
  int32_t horizontal_resolution{0};
  short vertical_resolution{0};
  /* 2byte下面需要再补充一个，如果置为0的话也可以不用写，编译器会通过字节对齐的方式跳过2个字节(下面是int_32)*/
  char _vertical_resolution{0}; /* 补充1byte */
  int32_t tab_index{0};
  int32_t important_idx{0};
} Info;
#pragma pack(pop)

class BMP {
 private:
  Head head;
  Info info;
  int _width = 0;
  int _height = 0;
  uint8_t *_rgbBuffer = nullptr;
  int _rgbBufferSize = 0;
  uint8_t *_yuv444PackedBuffer = nullptr;
  uint8_t *_yuv444PlanarBuffer = nullptr;
  int _YUVSize = 0;

 public:
  BMP();
  ~BMP();
  uint8_t *getRGBBuffer() { return _rgbBuffer; }
  int getRGBBufferSize() { return _rgbBufferSize; }
  uint8_t *getYUV444PackedBuffer() const { return _yuv444PackedBuffer; }
  uint8_t *getYUV444PlanarBuffer() const { return _yuv444PlanarBuffer; }
  int getYUVSize() { return _YUVSize; }
  int getWidth() { return _width; }
  int getHeight() { return _height; }

  int RGBToBMP(string &rgbFilePath, int width, int height, string &bmpFilePath);
  int BMPToRBG(string &bmpFilePath, string rgbFilePath = "");
  int BMPToYUV(string &bmpFilePath, string yuvFilePath = "");
  int close();
};

} // namespace File
#endif /* end of include guard: BMP_HPP_4ZTJJIQB */
