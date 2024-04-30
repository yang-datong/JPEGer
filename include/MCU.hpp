#ifndef MCU_HPP_MJKLNXG9
#define MCU_HPP_MJKLNXG9

#include "Common.hpp"
#include "Type.hpp"

class MCU {
 private:
  static int _MCUCount;
  static int _DCDiff[3];

  CompMatrices _matrix;
  IDCTCoeffs _idctCoeffs;
  DCTCoeffs _dctCoeffs;

  int _order = 0;
  array<vector<int>, 3> _RLE;
  const vector<QuantizationTable> _qtTables;

 public:
  MCU(CompMatrices &matrix, const vector<QuantizationTable> &qTables);
  MCU(array<vector<int>, 3> RLE, const vector<QuantizationTable> &qTables);

 private:
  void encodeACandDC();
  void decodeACandDC();
  inline void startDCT();
  inline void startIDCT();
  inline void levelShift();
  inline void performLevelShift();
  inline void YUVToRGB();

 public:
  const CompMatrices &getAllMatrices() const { return _matrix; }
};
#endif /* end of include guard: MCU_HPP_MJKLNXG9 */
