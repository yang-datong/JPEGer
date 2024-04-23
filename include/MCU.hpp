#ifndef MCU_HPP_MJKLNXG9
#define MCU_HPP_MJKLNXG9

#include "Common.hpp"
#include "Type.hpp"

enum RGBComponents { RED, GREEN, BLUE };
enum YUVComponents { Y, Cb, Cr };
enum OutputFileType { BMP, YUV, PPM, RGB };
// const int gOutputFileType = OutputFileType::YUV;
const int gOutputFileType = OutputFileType::PPM;

struct Pixel {
  int16_t comp[3] = {0};
};

/* 定义一个HuffmanTable数据结构 */
typedef array<pair<int, vector<uint8_t>>, HUFFMAN_CODE_LENGTH_POSSIBLE>
    HuffmanTable;

class MCU {
 public:
  MCU(array<vector<int>, 3> RLE, vector<vector<uint16_t>> qTables);

 private:
  static int _MCUCount;
  static int _DCDiff[3];

  int _order = 0;
  array<vector<int>, 3> _RLE;
  vector<vector<uint16_t>> _QTables;

  /* 3维矩阵 */
  typedef array<array<array<int, 8>, 8>, 3> CompMatrices;
  /* 3维逆DCT变化系数 */
  typedef array<array<array<float, 8>, 8>, 3> IDCTCoeffs;
  CompMatrices _matrix;
  IDCTCoeffs _idctCoeffs;

  void decodeACandDC();
  inline void startIDCT();
  inline void performLevelShift();
  inline void YUVToRGB();

 public:
  const CompMatrices &getAllMatrices() const { return _matrix; }
};
#endif /* end of include guard: MCU_HPP_MJKLNXG9 */
