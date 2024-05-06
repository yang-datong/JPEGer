#ifndef MCU_HPP_MJKLNXG9
#define MCU_HPP_MJKLNXG9

#include "Common.hpp"
#include "Type.hpp"

class MCU {
 private:
  static int _MCUCount;
  static int16_t _DCDiff[3];

  CompMatrices _matrix;
  UCompMatrices _Umatrix;
  IDCTCoeffs _idctCoeffs;
  DCTCoeffs _dctCoeffs;
  RLE _rle;

  vector<QuantizationTable> _qtTables;
  /* 编码时：经过了中心化操作后的宏块应该是包含负数的，所以应该使用int8_t，而不是uint8_t*/
  array<int16_t, MCU_UNIT_SIZE> _zzOrder = {0};
  /* 解码时：经过了还未解码的宏块应该是包含负数的，所以应该使用int8_t，而不是uint8_t*/

 public:
  MCU(UCompMatrices &matrix, const vector<QuantizationTable> &qTables);
  MCU(RLE rle, const vector<QuantizationTable> &qTables);

  void startEncode();
  void startDecode();

 private:
  void encodeACandDC();
  void decodeACandDC();
  void startDCT();
  void startIDCT();
  void levelShift();
  void performLevelShift();
  void fillACRLE(int imageComponent);
  void fillACRLE2(int imageComponent);

  void printZZOrder();
  void printMatrix();
  void printUmatrix();
  void printDCTCoeffs();
  void printIDCTCoeffs();
  void printRLE();

 public:
  /* 将反中心化的解码矩阵提供给外界创建图片等操作 */
  const UCompMatrices &getMatrices() const { return _Umatrix; }
  const RLE &getRLE() const { return _rle; }
};
#endif /* end of include guard: MCU_HPP_MJKLNXG9 */
