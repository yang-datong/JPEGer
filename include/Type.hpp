#ifndef TYPE_HPP_TPOWA9WD
#define TYPE_HPP_TPOWA9WD

#include <array>
#include <bitset>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iomanip> //用于格式化输出
#include <ios>
#include <iostream>
#include <iterator>
#include <math.h>
#include <memory>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <vector>

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
enum FileFormat { BMP, YUV, PPM, RGB };

struct Pixel {
  int16_t comp[3] = {0};
};

/* Huffman表 */
typedef array<pair<int, vector<uint8_t>>, HUFFMAN_CODE_LENGTH_POSSIBLE>
    HuffmanTable;

/* 量化表 */
typedef vector<uint16_t> QuantizationTable;

/* 3个二维矩阵(Y,U,V) */
typedef array<array<array<int, 8>, 8>, 3> CompMatrices;

/* 3个二维逆DCT系数 */
typedef array<array<array<float, 8>, 8>, 3> IDCTCoeffs;

#endif /* end of include guard: TYPE_HPP_TPOWA9WD */
