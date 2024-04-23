#ifndef QUANTIZATIONTABLE_HPP_UFNBGSXM
#define QUANTIZATIONTABLE_HPP_UFNBGSXM

#include "ByteStream.hpp"
#include "MCU.hpp"
#include <iomanip> //用于格式化输出

class QuantizationTable {
 public:
  /* ISO/IEC 10918-1 : 1993(E) : page 39 */
  int parseDQT(int index);
  typedef vector<uint16_t> quantizationTable;

 private:
  /* 这个函数不参与实际的解码(Option) */
  inline void printMatrix(int arr[8][8]);

  /* 这个函数不参与实际的解码(Option) */
  inline void encodeZigZag(int a[64], int matrix[8][8]);
};

#endif /* end of include guard: QUANTIZATIONTABLE_HPP_UFNBGSXM */
