#ifndef MCU_HPP_MJKLNXG9
#define MCU_HPP_MJKLNXG9

#include <array>
#include <fstream>
#include <iomanip> //用于格式化输出
#include <iostream>
#include <math.h>
#include <memory>
#include <vector>

#include <bitset>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <ios>
#include <iterator>
#include <netinet/in.h>
#include <sstream>
#include <string>

using namespace std;

#define COMPONENT_SIZE 8
#define MCU_UNIT_SIZE 8 * 8
#define QUANTIZATION_TAB_SIZE 8 * 8
/* 每个量化表的的大小为64字节 */
#define HUFFMAN_CODE_LENGTH_POSSIBLE 16
// Huffman code的长度定义为有16种可能

enum JEIF {
  SOI = 0xD8,
  APP0 = 0xE0,
  DQT = 0xDB,
  SOF0 = 0xC0,
  SOF2 = 0xC2,
  DHT = 0xC4,
  SOS = 0xDA,
  EOI = 0xD9,
  COM = 0xFE
};

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

  void arrayToMatrixUseZigZag();

  /* 3维矩阵 */
  typedef array<array<array<int, 8>, 8>, 3> CompMatrices;
  /* 3维逆DCT变化系数 */
  typedef array<array<array<float, 8>, 8>, 3> IDCTCoeffs;
  CompMatrices _matrix;
  IDCTCoeffs _idctCoeffs;

  void arrayToMatrixUseZigZag(const array<int, 64> a,
                              array<array<int, 8>, 8> &matrix);

  inline void printMatrix(const array<array<int, 8>, 8> matrix);
  inline void startIDCT();
  inline void performLevelShift();
  inline void YUVToRGB();

 public:
  const CompMatrices &getAllMatrices() const { return _matrix; }
};
#endif /* end of include guard: MCU_HPP_MJKLNXG9 */
